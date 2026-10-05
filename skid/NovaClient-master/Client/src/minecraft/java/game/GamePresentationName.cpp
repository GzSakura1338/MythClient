#include "minecraft/java/api/game_presentation_name.h"
#include "minecraft/compat/mappings/LunarNick.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/compat/Profile.h"
#include <windows.h>

namespace nova::java {
namespace {
jmethodID legacyFormatter=nullptr;
jobject LegacyNickModule(compat::mc263::access::Frame& f){
    // Verified in both Lunar legacy bakes: even 1.7 uses fontRendererObj.
    auto font=f.get(f.client(),{"net.minecraft.client.Minecraft","fontRendererObj","Lnet/minecraft/client/gui/FontRenderer;"});
    if(!font||!f.check())return nullptr;
    if(!legacyFormatter){
        auto* vmti=CurrentJvmti();if(!vmti)return nullptr;
        auto type=f.env->GetObjectClass(font);
        // Match the stable Mixin suffix, never its per-build generated prefix.
        while(type&&!legacyFormatter){
            jint count=0;jmethodID* methods=nullptr;
            if(vmti->GetClassMethods(type,&count,&methods)==JVMTI_ERROR_NONE){
                for(jint i=0;i<count;++i){char* method=nullptr;char* signature=nullptr;
                    if(vmti->GetMethodName(methods[i],&method,&signature,nullptr)==JVMTI_ERROR_NONE){
                        if(method&&signature&&std::string_view(method).ends_with("$lunar$renderString")&&
                            std::string_view(signature)=="(Ljava/lang/String;)Ljava/lang/String;")legacyFormatter=methods[i];
                    }
                    if(method)vmti->Deallocate(reinterpret_cast<unsigned char*>(method));
                    if(signature)vmti->Deallocate(reinterpret_cast<unsigned char*>(signature));
                }
                if(methods)vmti->Deallocate(reinterpret_cast<unsigned char*>(methods));
            }
            auto parent=legacyFormatter?nullptr:f.env->GetSuperclass(type);f.env->DeleteLocalRef(type);type=parent;
        }
    }
    return legacyFormatter?font:nullptr;
}
}
std::string PresentationName(std::string_view original){
    std::string fallback(original);
    if(original.empty()||original.size()>8192||(!compat::IsLegacyLunar()&&!compat::IsLunar12111()))return fallback;
    JniAttach attach;
    if(!attach||attach.env->ExceptionCheck())return fallback;
    compat::mc263::access::Frame f(attach.env);
    if(!f)return fallback;
    using namespace compat::lunarnick;
    // Same live Nick Hider formatter used by Lunar's text rendering. No cached
    // nickname: enabling/disabling and changing the name take effect naturally.
    const bool legacy=compat::IsLegacyLunar();
    jobject module=nullptr;
    Member formatter=Format;
    if(legacy)module=LegacyNickModule(f);
    else for(const auto& build:ModernBuilds){
        // Probe each verified naming build independently. Frame owns and clears
        // only exceptions raised by these optional JNI lookups.
        compat::mc263::access::Frame probe(attach.env);
        if(!probe)break;
        auto candidate=probe.call(probe.call(probe.staticCall(build.instance),build.modules),build.nickHider);
        if(candidate&&probe.check()&&compat::mc263::access::Method(probe.env,build.format)&&probe.check()){
            module=probe.keep(candidate);formatter=build.format;break;
        }
    }
    if(!module||!f.check())return fallback;
    const int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,original.data(),int(original.size()),nullptr,0);
    if(count<=0)return fallback;
    std::wstring wide(count,0);
    MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,original.data(),int(original.size()),wide.data(),count);
    const auto input=f.env->NewString(reinterpret_cast<const jchar*>(wide.data()),count);
    if(!input||!f.check())return fallback;
    const auto result=legacy?static_cast<jstring>(f.env->CallObjectMethod(module,legacyFormatter,input)):
        static_cast<jstring>(f.call(module,formatter,input));
    if(!result||!f.check())return fallback;
    const auto length=f.env->GetStringLength(result);
    if(length<=0||length>8192||!f.check())return fallback;
    const auto chars=f.env->GetStringChars(result,nullptr);
    if(!chars){f.check();return fallback;}
    const auto value=reinterpret_cast<const wchar_t*>(chars);
    const int bytes=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value,length,nullptr,0,nullptr,nullptr);
    std::string text;
    if(bytes>0&&bytes<=8192){text.resize(bytes);WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value,length,text.data(),bytes,nullptr,nullptr);}
    f.env->ReleaseStringChars(result,chars);
    return f.check()&&!text.empty()?text:fallback;
}
}
