#include "ui/clickgui/browser/BrowserBackdrop.h"
#include "ui/backend/blur/Gaussian.h"
#include <cmath>

namespace nova::ui::browser {
const chrome::Frame& BrowserBackdrop::Update(const chrome::Frame& source,blur::Options options) {
    const float scale=std::min(1.f,256.f/std::max(source.width,source.height));
    const int width=std::max(1,int(source.width*scale)),height=std::max(1,int(source.height*scale));
    const bool resized=frame_.width!=width||frame_.height!=height;
    if(!resized && refresh_.strength==options.strength &&
       (sourceSequence_==source.sequence||!refresh_.Due(options)))return frame_;
    frame_.width=width;frame_.height=height;frame_.rgba.resize(size_t(width)*height*4);scratch_.resize(frame_.rgba.size());
    // Four samples per cell avoid nearest-neighbour sparkle during page scrolling.
    for(int y=0;y<height;++y)for(int x=0;x<width;++x){
        int sum[3]{};
        for(int j=0;j<2;++j)for(int i=0;i<2;++i){
            const int sx=std::min(source.width-1,int((x+(i+.5f)*.5f)*source.width/width));
            const int sy=std::min(source.height-1,int((y+(j+.5f)*.5f)*source.height/height));
            const auto at=(size_t(sy)*source.width+sx)*4;
            for(int c=0;c<3;++c)sum[c]+=source.rgba[at+c];
        }
        const auto at=(size_t(y)*width+x)*4;
        for(int c=0;c<3;++c)frame_.rgba[at+c]=static_cast<unsigned char>(sum[c]/4);
        frame_.rgba[at+3]=255;
    }
    const blur::GaussianKernel kernel(options.strength);
    const int radius=kernel.radius;
    // Normalized separable Gaussian, matching the GPU kernel. This small CPU
    // surface is only refreshed for visible browser popups, with no GPU readback.
    auto pass=[&](bool horizontal){
        const int lines=horizontal?height:width,length=horizontal?width:height;
        auto at=[&](int line,int n){return size_t(horizontal?line*width+n:n*width+line)*4;};
        for(int line=0;line<lines;++line){
            for(int n=0;n<length;++n){
                const auto dst=at(line,n);
                float sum[3]{};
                for(int k=-radius;k<=radius;++k)for(int c=0;c<3;++c)
                    sum[c]+=frame_.rgba[at(line,std::clamp(n+k,0,length-1))+c]*kernel.weights[std::abs(k)];
                for(int c=0;c<3;++c)scratch_[dst+c]=static_cast<unsigned char>(std::clamp(std::lround(sum[c]),0l,255l));
                scratch_[dst+3]=255;
            }
        }
        frame_.rgba.swap(scratch_);
    };
    pass(true);pass(false);
    sourceSequence_=source.sequence;++frame_.sequence;refresh_.Updated(options);
    return frame_;
}
}
