#include "ui/backend/blur/Dx11Blur.h"
#include "ui/backend/blur/Gaussian.h"
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <vector>
#include <d3dcompiler.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "Blur test failed at %d: %s\n", __LINE__, #x); std::exit(1); } } while (false)
namespace {
using Microsoft::WRL::ComPtr;
float Half(std::uint16_t bits) {
    const int exponent = (bits >> 10) & 31;
    return std::ldexp(static_cast<float>((bits & 1023) + (exponent ? 1024 : 0)), exponent ? exponent - 25 : -24);
}
std::array<float, 4> Read(ID3D11Device* device, ID3D11DeviceContext* context, ImTextureID id) {
    CHECK(id);
    auto* view = reinterpret_cast<ID3D11ShaderResourceView*>(id);
    ComPtr<ID3D11Resource> resource; view->GetResource(&resource);
    ComPtr<ID3D11Texture2D> texture; CHECK(SUCCEEDED(resource.As(&texture)));
    D3D11_TEXTURE2D_DESC desc{}; texture->GetDesc(&desc);
    CHECK(desc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT);
    desc.Usage = D3D11_USAGE_STAGING; desc.BindFlags = 0; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> staging; CHECK(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &staging)));
    context->CopyResource(staging.Get(), texture.Get());
    D3D11_MAPPED_SUBRESOURCE map{}; CHECK(SUCCEEDED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &map)));
    const auto* row = reinterpret_cast<const std::uint16_t*>(static_cast<const char*>(map.pData) + (desc.Height / 2) * map.RowPitch);
    std::array<float, 4> result{Half(row[(desc.Width / 2 - 1) * 4]), Half(row[(desc.Width / 2) * 4]), Half(row[0]), Half(row[3])};
    context->Unmap(staging.Get(), 0); return result;
}
}
void TestBlur(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* chain) {
    nova::ui::blur::Dx11Blur blur;
    ComPtr<ID3D11Texture2D> backbuffer; CHECK(SUCCEEDED(chain->GetBuffer(0, IID_PPV_ARGS(&backbuffer))));
    D3D11_TEXTURE2D_DESC desc{}; backbuffer->GetDesc(&desc);
    std::vector<std::uint32_t> pixels(desc.Width * desc.Height);
    for (UINT y = 0; y < desc.Height; ++y) for (UINT x = 0; x < desc.Width; ++x)
        pixels[y * desc.Width + x] = x < desc.Width / 2 ? 0 : 0x00ffffff; // Deliberately zero alpha.
    auto sourceDesc = desc; sourceDesc.BindFlags = 0;
    D3D11_SUBRESOURCE_DATA initial{pixels.data(), desc.Width * 4, 0};
    ComPtr<ID3D11Texture2D> source; CHECK(SUCCEEDED(device->CreateTexture2D(&sourceDesc, &initial, &source)));
    context->CopyResource(backbuffer.Get(), source.Get());
    // A game-owned CS sampler must survive the blur passes.
    D3D11_SAMPLER_DESC sampling{}; sampling.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampling.AddressU = sampling.AddressV = sampling.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    ComPtr<ID3D11SamplerState> sampler; CHECK(SUCCEEDED(device->CreateSamplerState(&sampling, &sampler)));
    context->CSSetSamplers(0, 1, sampler.GetAddressOf());
    constexpr char dummyCode[] = "[numthreads(1,1,1)] void main(uint3 id:SV_DispatchThreadID) {}";
    ComPtr<ID3DBlob> code;
    CHECK(SUCCEEDED(D3DCompile(dummyCode, sizeof(dummyCode) - 1, nullptr, nullptr, nullptr, "main", "cs_5_0", 0, 0, &code, nullptr)));
    ComPtr<ID3D11ComputeShader> originalShader;
    CHECK(SUCCEEDED(device->CreateComputeShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &originalShader)));
    context->CSSetShader(originalShader.Get(), nullptr, 0);
    D3D11_BUFFER_DESC constantDesc{};constantDesc.ByteWidth=16;constantDesc.Usage=D3D11_USAGE_DEFAULT;
    constantDesc.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
    ComPtr<ID3D11Buffer> originalConstants;CHECK(SUCCEEDED(device->CreateBuffer(&constantDesc,nullptr,&originalConstants)));
    context->CSSetConstantBuffers(0,1,originalConstants.GetAddressOf());
    D3D11_TEXTURE2D_DESC sentinelDesc{}; sentinelDesc.Width = sentinelDesc.Height = 1;
    sentinelDesc.ArraySize = sentinelDesc.MipLevels = sentinelDesc.SampleDesc.Count = 1;
    sentinelDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    sentinelDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    ComPtr<ID3D11Texture2D> sentinelInput, sentinelOutput;
    CHECK(SUCCEEDED(device->CreateTexture2D(&sentinelDesc, nullptr, &sentinelInput)));
    CHECK(SUCCEEDED(device->CreateTexture2D(&sentinelDesc, nullptr, &sentinelOutput)));
    ComPtr<ID3D11ShaderResourceView> originalInput;
    ComPtr<ID3D11UnorderedAccessView> originalOutput;
    CHECK(SUCCEEDED(device->CreateShaderResourceView(sentinelInput.Get(), nullptr, &originalInput)));
    CHECK(SUCCEEDED(device->CreateUnorderedAccessView(sentinelOutput.Get(), nullptr, &originalOutput)));
    context->CSSetShaderResources(0, 1, originalInput.GetAddressOf());
    context->CSSetUnorderedAccessViews(0, 1, originalOutput.GetAddressOf(), nullptr);
    auto id = blur.Render(device, context, chain, {true, 1, 5});
    const auto sharp = Read(device, context, id);
    id = blur.Render(device, context, chain, {true, 6, 5});
    const auto soft = Read(device, context, id);
    // Half-resolution step edge, sampled at adjacent texel centers: the two
    // values must match the analytical normalized Gaussian cumulative weights.
    const nova::ui::blur::GaussianKernel weakKernel(1),strongKernel(6);
    CHECK(std::abs(sharp[0]-(1-weakKernel.weights[0])*.5f)<.003f);
    CHECK(std::abs(sharp[1]-(1+weakKernel.weights[0])*.5f)<.003f);
    CHECK(std::abs(soft[0]-(1-strongKernel.weights[0])*.5f)<.003f);
    CHECK(soft[0] > .1f && soft[0] < .5f && soft[1] > .5f && soft[1] < .9f);
    CHECK(std::abs(soft[0] + soft[1] - 1.f) < .02f && soft[3] == 1.f);
    ComPtr<ID3D11SamplerState> restored; context->CSGetSamplers(0, 1, &restored); CHECK(restored.Get() == sampler.Get());
    ComPtr<ID3D11ComputeShader> shader; context->CSGetShader(&shader, nullptr, nullptr); CHECK(shader.Get() == originalShader.Get());
    ComPtr<ID3D11Buffer> constants;context->CSGetConstantBuffers(0,1,&constants);CHECK(constants.Get()==originalConstants.Get());
    ComPtr<ID3D11ShaderResourceView> restoredInput; context->CSGetShaderResources(0, 1, &restoredInput); CHECK(restoredInput.Get() == originalInput.Get());
    ComPtr<ID3D11UnorderedAccessView> restoredOutput; context->CSGetUnorderedAccessViews(0, 1, &restoredOutput); CHECK(restoredOutput.Get() == originalOutput.Get());
    // A changed scene is retained until its configured refresh is due.
    std::fill(pixels.begin(), pixels.end(), 0x00ffffff);
    context->UpdateSubresource(source.Get(), 0, nullptr, pixels.data(), desc.Width * 4, 0);
    context->CopyResource(backbuffer.Get(), source.Get());
    const auto cached = Read(device, context, blur.Render(device, context, chain, {true, 6, 5}));
    CHECK(cached == soft);
    nova::ui::blur::Dx11Blur layer;
    const auto layerId=layer.Prepare(device,chain,{true,6,5});CHECK(layerId && layerId!=id);
    const auto live=Read(device,context,layer.Render(device,context,chain,{true,6,5},true));
    CHECK(live[0]==1.f && live[1]==1.f);
    ComPtr<ID3D11RenderTargetView> target;
    CHECK(SUCCEEDED(device->CreateRenderTargetView(backbuffer.Get(),nullptr,&target)));
    context->OMSetRenderTargets(1,target.GetAddressOf(),nullptr);
    auto* liveView=reinterpret_cast<ID3D11ShaderResourceView*>(layerId);
    context->PSSetShaderResources(0,1,&liveView);
    std::fill(pixels.begin(),pixels.end(),0u);
    context->UpdateSubresource(source.Get(),0,nullptr,pixels.data(),desc.Width*4,0);
    context->CopyResource(backbuffer.Get(),source.Get());
    const auto updated=Read(device,context,layer.Render(device,context,chain,{true,6,5},true));
    CHECK(updated[0]==0.f && updated[1]==0.f && updated[3]==1.f);
    ComPtr<ID3D11RenderTargetView> restoredTarget;context->OMGetRenderTargets(1,&restoredTarget,nullptr);
    CHECK(restoredTarget.Get()==target.Get());
    ComPtr<ID3D11ShaderResourceView> restoredPixel;context->PSGetShaderResources(0,1,&restoredPixel);
    CHECK(restoredPixel.Get()==liveView);
    CHECK(Read(device,context,id)==soft);
    ID3D11ShaderResourceView* noPixel=nullptr;context->PSSetShaderResources(0,1,&noPixel);
    context->OMSetRenderTargets(0,nullptr,nullptr);layer.Release();
    std::fill(pixels.begin(),pixels.end(),0x00ffffff);
    context->UpdateSubresource(source.Get(),0,nullptr,pixels.data(),desc.Width*4,0);
    context->CopyResource(backbuffer.Get(),source.Get());
    CHECK(blur.Render(device, context, chain, {false, 6, 5}) == 0);
    const auto fresh = Read(device, context, blur.Render(device, context, chain, {true, 6, 5}));
    CHECK(fresh[0] == 1.f && fresh[1] == 1.f && fresh[3] == 1.f);
    blur.Resize(); CHECK(blur.Render(device, context, chain, {true, 3, 30}) != 0);
    blur.Release();
    ID3D11SamplerState* empty = nullptr; context->CSSetSamplers(0, 1, &empty);
    ID3D11ShaderResourceView* noInput = nullptr; context->CSSetShaderResources(0, 1, &noInput);
    ID3D11UnorderedAccessView* noOutput = nullptr; context->CSSetUnorderedAccessViews(0, 1, &noOutput, nullptr);
    context->CSSetShader(nullptr, nullptr, 0);
    ID3D11Buffer* noConstants=nullptr;context->CSSetConstantBuffers(0,1,&noConstants);
    puts("PASS: DX11 blur GPU pixels, strength, opaque alpha, refresh cache, toggle, resource rebuild, compute-state restoration");
}
