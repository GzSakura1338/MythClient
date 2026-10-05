#include "ui/clickgui/panel/PopupAnimation.h"
#include <algorithm>
#include <cmath>
#include "imgui_internal.h"
#include "ui/clickgui/panel/Widgets.h"
#include "ui/clickgui/panel/ReferenceStyle.h"
#include "utils/Color.h"
#include "ui/backend/blur/Blur.h"

namespace nova::menu::panel {
namespace {
PopupAnimations* animations = nullptr;
float Ease(float reveal, PopupMotion motion) {
    return motion == PopupMotion::Account ? reveal : reference::PopupEase(reveal);
}
ImVec2 Transform(ImVec2 p, ImVec2 position, ImVec2 size, float e, PopupMotion motion) {
    if(motion==PopupMotion::Account)return p;
    const auto pivot=position+size*.5f;
    return pivot+(p-pivot)*ImLerp(.96f,1.f,e)+ImVec2(Scale(4)*(1-e),0);
}
}
void SetPopupAnimations(PopupAnimations* value) { animations=value; }
void CapturePopupAnimation(ImGuiWindow* window,float reveal,PopupMotion motion) {
    if(animations)animations->capture(window,reveal,motion);
}
void TransformPopup(ImDrawList* list,ImVec2 position,ImVec2 size,float reveal,PopupMotion motion) {
    const float e=Ease(reveal,motion);
    for(auto& v:list->VtxBuffer) {
        v.pos=Transform(v.pos,position,size,e,motion);
        v.col=color::ReAlpha(v.col,int(color::Alpha(v.col)*e));
    }
    for(auto& cmd:list->CmdBuffer) {
        if(!cmd.UserCallback && !cmd.TexRef._TexData && ui::blur::IsBackdropTexture(cmd.TexRef._TexID))
            for(unsigned i=0;i<cmd.ElemCount;++i) {
                auto& vertex=list->VtxBuffer[cmd.VtxOffset+list->IdxBuffer[cmd.IdxOffset+i]];
                vertex.uv=ui::blur::BackdropUv(vertex.pos);
            }
        auto a=Transform({cmd.ClipRect.x,cmd.ClipRect.y},position,size,e,motion);
        auto b=Transform({cmd.ClipRect.z,cmd.ClipRect.w},position,size,e,motion);
        cmd.ClipRect={a.x,a.y,b.x,b.y};
    }
}
void PopupAnimations::reset() { entries_.clear(); }
void PopupAnimations::capture(ImGuiWindow* root,float reveal,PopupMotion motion) {
    auto found=std::find_if(entries_.begin(),entries_.end(),[&](const Entry& e){return e.id==root->ID;});
    if(found==entries_.end()){entries_.emplace_back();found=entries_.end()-1;}
    auto& e=*found;
    e.id=root->ID;e.frame=ImGui::GetFrameCount();e.reveal=reveal;e.motion=motion;e.position=root->Pos;e.size=root->Size;
    e.lists.clear();
    for(auto* w:GImGui->Windows)if(w->Active&&(w==root||ImGui::IsWindowChildOf(w,root,false)))
        e.lists.emplace_back(w->DrawList->CloneOutput(),[](ImDrawList* list){IM_DELETE(list);});
}
void PopupAnimations::drawClosing() {
    auto* out=ImGui::GetForegroundDrawList();
    for(auto& entry:entries_) {
        if(entry.frame==ImGui::GetFrameCount())continue;
        entry.reveal*=std::exp(-(entry.motion==PopupMotion::Account?17.f:20.f)*ImGui::GetIO().DeltaTime);
        const float e=Ease(entry.reveal,entry.motion);
        if(entry.reveal<.002f)continue;
        for(const auto& list:entry.lists)for(const auto& cmd:list->CmdBuffer) {
            // Callbacks may contain frame-local backend pointers. Never replay them.
            if(cmd.UserCallback||cmd.ElemCount==0)continue;
            const auto a=Transform({cmd.ClipRect.x,cmd.ClipRect.y},entry.position,entry.size,e,entry.motion);
            const auto b=Transform({cmd.ClipRect.z,cmd.ClipRect.w},entry.position,entry.size,e,entry.motion);
            const bool backdrop=!cmd.TexRef._TexData && ui::blur::IsBackdropTexture(cmd.TexRef._TexID);
            if(backdrop && !ui::blur::Available())continue;
            out->PushClipRect(a,b,false);
            out->PushTexture(backdrop?ImTextureRef(ui::blur::CaptureLayerBackdrop(out)):cmd.TexRef);
            // Rebase in bounded batches, supporting 16-bit ImDrawIdx at any menu scale.
            for(unsigned offset=0;offset<cmd.ElemCount;) {
                const unsigned count=std::min(3000u,cmd.ElemCount-offset);
                out->PrimReserve(count,count);
                for(unsigned i=0;i<count;++i) {
                    const auto& v=list->VtxBuffer[cmd.VtxOffset+list->IdxBuffer[cmd.IdxOffset+offset+i]];
                    out->PrimWriteIdx(static_cast<ImDrawIdx>(out->_VtxCurrentIdx));
                    const auto p=Transform(v.pos,entry.position,entry.size,e,entry.motion);
                    out->PrimWriteVtx(p,backdrop?ui::blur::BackdropUv(p):v.uv,color::ReAlpha(v.col,int(color::Alpha(v.col)*e)));
                }
                offset+=count;
            }
            out->PopTexture();out->PopClipRect();
        }
    }
    std::erase_if(entries_,[](const Entry& e){return e.reveal<.002f;});
}
}
