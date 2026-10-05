#include "ui/backend/blur/Dx11Blur.h"
#include "ui/backend/blur/Gaussian.h"
#include <d3dcompiler.h>
#include <cstring>

namespace nova::ui::blur {
namespace {
using Microsoft::WRL::ComPtr;
// Preserve the stored scene colors, matching the non-sRGB GL blur cache.
DXGI_FORMAT SamplingFormat(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_R8G8B8A8_TYPELESS: case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS: case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_B8G8R8X8_TYPELESS: case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8X8_UNORM;
    default: return format;
    }
}
// Unbind the live cache from ImGui's PS slot and the scene from OM while
// copying/filtering, then restore them along with the compute state.
struct ComputeState {
    ID3D11DeviceContext* context;
    ComPtr<ID3D11ComputeShader> shader;
    ComPtr<ID3D11ShaderResourceView> input;
    ComPtr<ID3D11UnorderedAccessView> output;
    ComPtr<ID3D11SamplerState> sampler;
    ComPtr<ID3D11Buffer> parameters;
    ComPtr<ID3D11ShaderResourceView> pixelInput;
    ID3D11RenderTargetView* targets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
    ComPtr<ID3D11DepthStencilView> depth;
    ID3D11ClassInstance* instances[256]{};
    UINT count = 256;
    explicit ComputeState(ID3D11DeviceContext* value) : context(value) {
        context->CSGetShader(shader.GetAddressOf(), instances, &count);
        context->CSGetShaderResources(0, 1, input.GetAddressOf());
        context->CSGetUnorderedAccessViews(0, 1, output.GetAddressOf());
        context->CSGetSamplers(0, 1, sampler.GetAddressOf());
        context->CSGetConstantBuffers(0, 1, parameters.GetAddressOf());
        context->PSGetShaderResources(0, 1, pixelInput.GetAddressOf());
        context->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, targets, depth.GetAddressOf());
        ID3D11ShaderResourceView* empty = nullptr;
        context->PSSetShaderResources(0, 1, &empty);
        context->OMSetRenderTargets(0, nullptr, nullptr);
    }
    ~ComputeState() {
        ID3D11ShaderResourceView* emptyInput = nullptr;
        ID3D11UnorderedAccessView* emptyOutput = nullptr;
        context->CSSetShaderResources(0, 1, &emptyInput);
        context->CSSetUnorderedAccessViews(0, 1, &emptyOutput, nullptr);
        context->CSSetShader(shader.Get(), instances, count);
        context->CSSetShaderResources(0, 1, input.GetAddressOf());
        constexpr UINT preserveCounter = ~0u;
        context->CSSetUnorderedAccessViews(0, 1, output.GetAddressOf(), &preserveCounter);
        context->CSSetSamplers(0, 1, sampler.GetAddressOf());
        context->CSSetConstantBuffers(0, 1, parameters.GetAddressOf());
        context->PSSetShaderResources(0, 1, pixelInput.GetAddressOf());
        context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, targets, depth.Get());
        for (auto* target : targets) if (target) target->Release();
        for (UINT i = 0; i < count; ++i) if (instances[i]) instances[i]->Release();
    }
};
}
void Dx11Blur::Resize() {
    levels_ = {}; refresh_ = {}; format_ = DXGI_FORMAT_UNKNOWN; samples_ = 0; failed_ = false;
}
void Dx11Blur::Release() { Resize(); shader_.Reset(); sampler_.Reset(); parameters_.Reset(); }
bool Dx11Blur::Pipeline(ID3D11Device* device) {
    if (shader_ && sampler_ && parameters_) return true;
    constexpr char source[] = R"(
Texture2D<float4> scene : register(t0);
SamplerState linearClamp : register(s0);
RWTexture2D<float4> filtered : register(u0);
cbuffer Gaussian : register(b0) { float2 direction; int count; float padding; float4 pairs[5]; };
float2 tap(int i) { return (i & 1) ? pairs[i/2].zw : pairs[i/2].xy; }
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    uint width, height; filtered.GetDimensions(width, height);
    if (id.x >= width || id.y >= height) return;
    float2 uv = (float2(id.xy) + 0.5) / float2(width, height);
    float3 color=scene.SampleLevel(linearClamp,uv,0).rgb*tap(0).x;
    for(int i=1;i<count;++i) {
        float2 t=tap(i);float2 offset=direction*t.y;
        color+=(scene.SampleLevel(linearClamp,uv+offset,0).rgb+scene.SampleLevel(linearClamp,uv-offset,0).rgb)*t.x;
    }
    filtered[id.xy] = float4(color, 1.0);
})";
    ComPtr<ID3DBlob> code, error;
    if (FAILED(D3DCompile(source, sizeof(source) - 1, "Nova.Dx11Blur", nullptr, nullptr, "main", "cs_5_0",
                         D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &code, &error))) return false;
    if (FAILED(device->CreateComputeShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, shader_.ReleaseAndGetAddressOf()))) return false;
    D3D11_SAMPLER_DESC desc{};
    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    desc.MaxLOD = D3D11_FLOAT32_MAX;
    if(FAILED(device->CreateSamplerState(&desc, sampler_.ReleaseAndGetAddressOf())))return false;
    D3D11_BUFFER_DESC buffer{};buffer.ByteWidth=sizeof(GaussianPass);
    buffer.Usage=D3D11_USAGE_DEFAULT;buffer.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
    return SUCCEEDED(device->CreateBuffer(&buffer,nullptr,parameters_.ReleaseAndGetAddressOf()));
}
bool Dx11Blur::Allocate(ID3D11Device* device, D3D11_TEXTURE2D_DESC source) {
    const auto format = SamplingFormat(source.Format);
    if (levels_[0].texture && levels_[0].width == source.Width && levels_[0].height == source.Height &&
        format_ == format && samples_ == source.SampleDesc.Count) return true;
    Resize();
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = source.Width; desc.Height = source.Height;
    desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
    desc.Format = format; desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    for (size_t i = 0; i < levels_.size(); ++i) {
        auto& level = levels_[i];
        level.width = desc.Width; level.height = desc.Height;
        if (FAILED(device->CreateTexture2D(&desc, nullptr, &level.texture)) ||
            FAILED(device->CreateShaderResourceView(level.texture.Get(), nullptr, &level.view)) ||
            (i && FAILED(device->CreateUnorderedAccessView(level.texture.Get(), nullptr, &level.output)))) {
            Resize(); return false;
        }
        if(i==0){desc.Width = std::max(1u, desc.Width / 2); desc.Height = std::max(1u, desc.Height / 2);}
        // Typed UAV stores are supported for RGBA16F even when the backbuffer is BGRA.
        desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
    }
    format_ = format; samples_ = source.SampleDesc.Count;
    return true;
}
ImTextureID Dx11Blur::Prepare(ID3D11Device* device, IDXGISwapChain* chain, Options options) {
    if (!options.enabled) { refresh_.valid = false; return 0; }
    if (!device || !chain || failed_) return 0;
    if (!Pipeline(device)) { failed_ = true; return 0; }
    ComPtr<ID3D11Texture2D> buffer;
    if (FAILED(chain->GetBuffer(0, IID_PPV_ARGS(&buffer)))) return 0;
    D3D11_TEXTURE2D_DESC source{}; buffer->GetDesc(&source);
    if (!source.Width || !source.Height) return 0;
    if (!Allocate(device, source)) { failed_ = true; return 0; }
    return reinterpret_cast<ImTextureID>(levels_[3].view.Get());
}
ImTextureID Dx11Blur::Render(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* chain, Options options, bool force) {
    const auto texture = Prepare(device, chain, options);
    if (!texture || !context) return 0;
    ComPtr<ID3D11Texture2D> buffer;
    if (FAILED(chain->GetBuffer(0, IID_PPV_ARGS(&buffer)))) return 0;
    if (force || refresh_.Due(options)) {
        ComputeState restore(context);
        if (samples_ > 1) context->ResolveSubresource(levels_[0].texture.Get(), 0, buffer.Get(), 0, format_);
        else context->CopyResource(levels_[0].texture.Get(), buffer.Get());
        context->CSSetShader(shader_.Get(), nullptr, 0);
        context->CSSetSamplers(0, 1, sampler_.GetAddressOf());
        context->CSSetConstantBuffers(0,1,parameters_.GetAddressOf());
        auto pass=[&](int from,int to,const GaussianPass& params) {
            context->UpdateSubresource(parameters_.Get(),0,nullptr,&params,0,0);
            auto& output = levels_[to];
            context->CSSetShaderResources(0, 1, levels_[from].view.GetAddressOf());
            context->CSSetUnorderedAccessViews(0, 1, output.output.GetAddressOf(), nullptr);
            context->Dispatch((output.width + 7) / 8, (output.height + 7) / 8, 1);
            ID3D11ShaderResourceView* emptyInput = nullptr;
            ID3D11UnorderedAccessView* emptyOutput = nullptr;
            context->CSSetShaderResources(0, 1, &emptyInput);
            context->CSSetUnorderedAccessViews(0, 1, &emptyOutput, nullptr);
        };
        pass(0,1,GaussianPass{});
        pass(1,2,GaussianPass(options.strength,1.f/levels_[1].width,0));
        pass(2,3,GaussianPass(options.strength,0,1.f/levels_[1].height));
        refresh_.Updated(options);
    }
    return texture;
}
}
