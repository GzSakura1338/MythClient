#include "minecraft/java/api/game_victory.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include <Windows.h>

namespace nova::java {
namespace {
using compat::mc263::access::Frame;
using compat::mc263::access::Member;
struct Mapping { Member chat,title,subtitle,actionbar,kind,overlay,text; };
#define NOVA_VICTORY_MAPPING
#define NOVA_VICTORY(id,owner,name,desc) .id={owner,name,desc},
constexpr Mapping lunar{
#include "minecraft/compat/mappings/Lunar189.inc"
};
constexpr Mapping forge{
#include "minecraft/compat/mappings/forge1201.inc"
};
constexpr Mapping modern{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_VICTORY
#undef NOVA_VICTORY_MAPPING
}
bool ReadAnnouncement(jobject packet,Announcement& out) {
    out={};Frame f;if(!f || !packet)return false;
    const auto& m=compat::IsLegacyLunar()?lunar:compat::UsesModernRuntime()?modern:forge;
    jobject component=nullptr;
    if(f.instance(packet,m.chat.owner)) {
        component=f.call(packet,m.chat);
        // Legacy chat and action bar share S02. Keep both, and filter formatted
        // player chat in the matcher. Modern signed/player chat is not read.
        if(!compat::IsLegacyLunar() && f.call<bool>(packet,m.overlay))out.source=AnnouncementSource::ActionBar;
    } else if(!compat::IsLunar1710() && f.instance(packet,m.title.owner)) {
        if(compat::IsLegacyLunar()) {
            auto type=f.call(packet,m.kind);
            const auto name=f.text(f.call(type,{"java.lang.Enum","name","()Ljava/lang/String;"}));
            if(name!="TITLE" && name!="SUBTITLE")return false;
            out.source=name=="TITLE"?AnnouncementSource::Title:AnnouncementSource::Subtitle;
        } else out.source=AnnouncementSource::Title;
        component=f.call(packet,m.title);
    } else if(!compat::IsLegacyLunar() && f.instance(packet,m.subtitle.owner)) {
        component=f.call(packet,m.subtitle);out.source=AnnouncementSource::Subtitle;
    } else if(!compat::IsLegacyLunar() && f.instance(packet,m.actionbar.owner)) {
        component=f.call(packet,m.actionbar);out.source=AnnouncementSource::ActionBar;
    }
    if(!component || !f.check())return false;
    const auto value=static_cast<jstring>(f.call(component,m.text));
    if(!value || !f.check())return false;
    const auto length=f.env->GetStringLength(value);
    if(length<=0 || length>8192)return false;
    const auto chars=f.env->GetStringChars(value,nullptr);
    if(!chars){f.check();return false;}
    // JNI modified UTF-8 cannot represent emoji as standard UTF-8. Convert
    // UTF-16 explicitly so decorated victory titles still reach the matcher.
    const auto wide=reinterpret_cast<const wchar_t*>(chars);
    const int bytes=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,wide,length,nullptr,0,nullptr,nullptr);
    if(bytes>0 && bytes<=8192) {
        out.text.resize(bytes);
        WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,wide,length,out.text.data(),bytes,nullptr,nullptr);
    }
    f.env->ReleaseStringChars(value,chars);
    return f.check() && !out.text.empty() && out.text.size()<=8192;
}
}
