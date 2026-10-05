#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <array>
#include "ui/backend/blur/Blur.h"

namespace nova::ui::blur {
// Independent caches for the world and the already-composited UI layers.
class Dx11Blur {
public:
    ImTextureID Render(ID3D11Device* device, ID3D11DeviceContext* context,
                       IDXGISwapChain* chain, Options options, bool force = false);
    ImTextureID Prepare(ID3D11Device* device, IDXGISwapChain* chain, Options options);
    void Release();
    void Resize();
private:
    template<class T> using Ptr = Microsoft::WRL::ComPtr<T>;
    struct Level {
        Ptr<ID3D11Texture2D> texture;
        Ptr<ID3D11ShaderResourceView> view;
        Ptr<ID3D11UnorderedAccessView> output;
        UINT width = 0, height = 0;
    };
    bool Pipeline(ID3D11Device* device);
    bool Allocate(ID3D11Device* device, D3D11_TEXTURE2D_DESC source);
    std::array<Level, 4> levels_; // full-size resolve, half-size input, horizontal, vertical
    Ptr<ID3D11ComputeShader> shader_;
    Ptr<ID3D11SamplerState> sampler_;
    Ptr<ID3D11Buffer> parameters_;
    Refresh refresh_;
    DXGI_FORMAT format_ = DXGI_FORMAT_UNKNOWN;
    UINT samples_ = 0;
    bool failed_ = false;
};
}
