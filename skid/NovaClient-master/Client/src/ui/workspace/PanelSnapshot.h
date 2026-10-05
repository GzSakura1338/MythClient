#pragma once
#include "imgui_internal.h"
#include "ui/backend/blur/Blur.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <memory>
#include <unordered_map>
#include <vector>

namespace nova::ui::workspace {
// Freeze CPU-backed images (including the font atlas) for the transition.
// Blur is rebound to this frame's backdrop; arbitrary callbacks are never replayed.
class PanelSnapshot {
    struct List {
        std::shared_ptr<ImDrawList> draws;
        std::vector<bool> backdrop;
        std::vector<int> fringeAnchor;
    };
    struct Texture {
        std::unique_ptr<ImTextureData> data;
        int retired=-1;
        ImTextureData* parent=nullptr;
        int level=0;
    };
    std::vector<List> lists_;
    std::vector<Texture> textures_;
    ImVec2 origin_{},size_{1,1};
    bool frozen_=false;
public:
    // Called after backend shutdown, before the ImGui context is destroyed.
    void release(){
        frozen_=false;
        lists_.clear();
        for(auto& texture:textures_)ImGui::UnregisterUserTexture(texture.data.get());
        textures_.clear();
    }
    void collect(){
        for(auto it=textures_.begin();it!=textures_.end();){
            auto& texture=*it->data;
            if(it->retired>=0){
                texture.UnusedFrames=ImGui::GetFrameCount()-it->retired;
                if(texture.UnusedFrames>0)texture.SetStatus(ImTextureStatus_WantDestroy);
                if(texture.UnusedFrames>0&&texture.TexID==ImTextureID_Invalid&&!texture.BackendUserData){
                    ImGui::UnregisterUserTexture(&texture);it=textures_.erase(it);continue;
                }
            }
            ++it;
        }
    }
    void clear(){
        frozen_=false;
        lists_.clear();
        for(auto& texture:textures_)if(texture.retired<0){
            texture.retired=ImGui::GetFrameCount();texture.data->WantDestroyNextFrame=true;
        }
        collect();
    }
    bool empty()const{return lists_.empty();}
    void capture(ImGuiWindow* root){
        clear();origin_=root->Pos;size_=root->Size;
        for(auto* window:GImGui->Windows)
            if((window->Active||window->WasActive) && window->RootWindow==root && !(window->Flags&ImGuiWindowFlags_Popup)){
                List list{std::shared_ptr<ImDrawList>(window->DrawList->CloneOutput(),[](ImDrawList* p){IM_DELETE(p);}),{}};
                for(auto& cmd:list.draws->CmdBuffer){
                    const bool backdrop=!cmd.TexRef._TexData&&blur::IsBackdropTexture(cmd.TexRef._TexID);
                    list.backdrop.push_back(backdrop);
                }
                // Keep transparent AA fringe vertices one screen pixel from
                // their connected opaque edge after shrinking the snapshot.
                list.fringeAnchor.assign(list.draws->VtxBuffer.Size,-1);
                for(const auto& cmd:list.draws->CmdBuffer)for(unsigned i=0;i+2<cmd.ElemCount;i+=3){
                    unsigned ids[3];
                    for(int j=0;j<3;++j)ids[j]=cmd.VtxOffset+list.draws->IdxBuffer[cmd.IdxOffset+i+j];
                    for(int j=0;j<3;++j){
                        const auto& v=list.draws->VtxBuffer[ids[j]];
                        if(v.col&IM_COL32_A_MASK)continue;
                        auto& anchor=list.fringeAnchor[ids[j]];
                        for(int k=0;k<3;++k){
                            const auto& other=list.draws->VtxBuffer[ids[k]];
                            if((other.col&IM_COL32_A_MASK)!=IM_COL32_A_MASK)continue;
                            const float distance=ImLengthSqr(v.pos-other.pos);
                            if(distance>4.f||distance<.01f)continue;
                            if(anchor<0||distance<ImLengthSqr(v.pos-list.draws->VtxBuffer[anchor].pos))anchor=int(ids[k]);
                        }
                    }
                }
                lists_.push_back(std::move(list));
            }
    }
    void freeze(){
        // Reversing an unfinished restore reuses this snapshot. Copying its
        // already-owned atlas again loses the white-texel minification fix and
        // allocates another texture pyramid on every toggle.
        if(frozen_||empty())return;
        std::unordered_map<ImTextureData*,ImTextureRef> copies;
        for(auto& list:lists_)for(int index=0;index<list.draws->CmdBuffer.Size;++index){
                    auto& cmd=list.draws->CmdBuffer[index];
                    if(list.backdrop[index]||cmd.UserCallback||!cmd.ElemCount)continue;
                    auto* source=cmd.TexRef._TexData;
                    // Only managed, CPU-backed textures can be safely retained.
                    const auto& registered=ImGui::GetPlatformIO().Textures;
                    if(!source||std::find(registered.begin(),registered.end(),source)==registered.end()||!source->Pixels){cmd.ElemCount=0;continue;}
                    auto found=copies.find(source);
                    if(found==copies.end()){
                        auto copy=std::make_unique<ImTextureData>();
                        copy->Create(source->Format,source->Width,source->Height);
                        std::memcpy(copy->Pixels,source->Pixels,source->GetSizeInBytes());
                        ImGui::RegisterUserTexture(copy.get());
                        found=copies.emplace(source,copy->GetTexRef()).first;
                        auto* base=copy.get();
                        textures_.push_back({std::move(copy)});
                        auto* previous=base;
                        // Area-filtered levels avoid point-like sampling of
                        // full-size glyphs and page images on a small card.
                        for(int level=1;level<=2&&previous->Width>1&&previous->Height>1;++level){
                            auto smaller=std::make_unique<ImTextureData>();
                            smaller->Create(previous->Format,std::max(1,previous->Width/2),std::max(1,previous->Height/2));
                            const int channels=previous->BytesPerPixel;
                            for(int y=0;y<smaller->Height;++y)for(int x=0;x<smaller->Width;++x){
                                unsigned sum[4]{},weight=0,count=0;
                                const int x0=x*previous->Width/smaller->Width,x1=(x+1)*previous->Width/smaller->Width;
                                const int y0=y*previous->Height/smaller->Height,y1=(y+1)*previous->Height/smaller->Height;
                                for(int sy=y0;sy<y1;++sy)for(int sx=x0;sx<x1;++sx){
                                    const auto* pixel=previous->Pixels+(sy*previous->Width+sx)*channels;
                                    const unsigned alpha=channels==4?pixel[3]:255;
                                    for(int c=0;c<channels;++c)sum[c]+=pixel[c]*(channels==4&&c<3?alpha:1);
                                    weight+=alpha;++count;
                                }
                                auto* pixel=smaller->Pixels+(y*smaller->Width+x)*channels;
                                for(int c=0;c<channels;++c)pixel[c]=static_cast<unsigned char>(sum[c]/std::max(1u,channels==4&&c<3?weight:count));
                            }
                            // Solid geometry samples the atlas white texel.
                            // Minification must not turn that texel translucent.
                            if(source==ImGui::GetIO().Fonts->TexRef._TexData){
                                const auto uv=ImGui::GetIO().Fonts->TexUvWhitePixel;
                                const int cx=int(uv.x*smaller->Width-.5f),cy=int(uv.y*smaller->Height-.5f);
                                for(int y=std::max(0,cy);y<=std::min(smaller->Height-1,cy+1);++y)
                                    for(int x=std::max(0,cx);x<=std::min(smaller->Width-1,cx+1);++x)
                                        std::memset(smaller->Pixels+(y*smaller->Width+x)*channels,255,channels);
                            }
                            ImGui::RegisterUserTexture(smaller.get());previous=smaller.get();
                            textures_.push_back({std::move(smaller),-1,base,level});
                        }
                    }
                    cmd.TexRef=found->second;
        }
        frozen_=true;
    }
    void draw(ImDrawList* out,ImVec2 pos,ImVec2 size,ImU32 background)const{
        if(empty()){blur::LayerBackground(out,pos,pos+size,background,std::min(12.f,size.x*.025f));return;}
        const auto scale=size/size_;
        const int level=std::clamp(int(std::floor(std::log2(1.f/std::max(.01f,std::min(scale.x,scale.y))))),0,2);
        auto transform=[&](ImVec2 p){return pos+(p-origin_)*scale;};
        if(const auto linear=ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear)out->AddCallback(linear,nullptr);
        for(const auto& saved:lists_)for(int index=0;index<saved.draws->CmdBuffer.Size;++index){
            const auto& list=saved.draws;
            const auto& cmd=list->CmdBuffer[index];
            const bool backdrop=saved.backdrop[index];
            if(cmd.UserCallback || !cmd.ElemCount || (backdrop&&!blur::Available()))continue;
            auto a=ImMax(pos,transform({cmd.ClipRect.x,cmd.ClipRect.y}));
            auto b=ImMin(pos+size,transform({cmd.ClipRect.z,cmd.ClipRect.w}));
            if(a.x>=b.x||a.y>=b.y)continue;
            auto texture=cmd.TexRef;
            if(!backdrop&&level>0)for(const auto& candidate:textures_)
                if(candidate.retired<0&&candidate.parent==texture._TexData&&candidate.level==level){texture=candidate.data->GetTexRef();break;}
            out->PushClipRect(a-ImVec2(1,1),b+ImVec2(1,1),true);out->PushTexture(backdrop?ImTextureRef(blur::CaptureLayerBackdrop(out)):texture);
            for(unsigned offset=0;offset<cmd.ElemCount;){
                const unsigned count=std::min(3000u,cmd.ElemCount-offset);
                out->PrimReserve(count,count);
                for(unsigned i=0;i<count;++i){
                    const auto index=cmd.VtxOffset+list->IdxBuffer[cmd.IdxOffset+offset+i];
                    const auto& v=list->VtxBuffer[index];
                    out->PrimWriteIdx(static_cast<ImDrawIdx>(out->_VtxCurrentIdx));
                    auto p=transform(v.pos);
                    const int anchor=saved.fringeAnchor[index];
                    if(!backdrop&&anchor>=0){
                        const auto inner=transform(list->VtxBuffer[anchor].pos),delta=p-inner;
                        const float length=std::sqrt(ImLengthSqr(delta));
                        if(length>.001f&&length<1.f)p=inner+delta/length;
                    }
                    out->PrimWriteVtx(p,backdrop?blur::BackdropUv(p):v.uv,v.col);
                }
                offset+=count;
            }
            out->PopTexture();out->PopClipRect();
        }
    }
};
}
