#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace nova::ui::blur {
// Sigma is measured in half-resolution texels (2..12 full-size pixels).
// Truncate at three sigma and normalize so flat colors retain their brightness.
struct GaussianKernel {
    std::array<float,19> weights{};
    int radius=0;
    explicit GaussianKernel(int strength) {
        const float sigma=float(std::clamp(strength,1,6));radius=int(sigma)*3;
        float sum=0;
        for(int i=0;i<=radius;++i){weights[i]=std::exp(-float(i*i)/(2*sigma*sigma));sum+=weights[i]*(i?2:1);}
        for(auto& w:weights)w/=sum;
    }
};
// 96 bytes: GLSL push constants / HLSL cbuffer share five packed float4 pairs.
// Pair adjacent weights using bilinear sampling: exactly the discrete Gaussian
// convolution at texel centers, with at most 19 fetches per axis instead of 37.
struct GaussianPass {
    float x=0,y=0;
    int count=1;
    float padding=0;
    std::array<std::array<float,2>,10> taps{{{1,0}}}; // weight, texel offset
    GaussianPass()=default; // Copy/downsample pass.
    GaussianPass(int strength,float dx,float dy):x(dx),y(dy) {
        GaussianKernel kernel(strength);taps[0]={kernel.weights[0],0};
        for(int i=1;i<=kernel.radius;i+=2) {
            const float a=kernel.weights[i],b=i+1<=kernel.radius?kernel.weights[i+1]:0;
            taps[count++]={a+b,float(i)+b/(a+b)};
        }
    }
};
static_assert(sizeof(GaussianPass)==96);
}
