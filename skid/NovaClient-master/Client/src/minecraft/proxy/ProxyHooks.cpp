#include "minecraft/proxy/ProxyHooks.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/java/api/jni_lookup.h"
#include "proxy/ProxyService.h"
#include "proxy/TargetAddress.h"
#include <mutex>
#include <vector>
#include <memory>
#include <stdexcept>
#include <atomic>
#include <algorithm>

namespace nova::minecraft::proxyadapter {
namespace {
DWORD slot=TLS_OUT_OF_INDEXES;
struct Join {jobject screen=nullptr;jfieldID cancel=nullptr;proxy::Config config;std::string requestedHost;
    ~Join(){if(!config.password.empty())SecureZeroMemory(config.password.data(),config.password.size());}
};
std::atomic<bool> stopping{false};
std::mutex joinMutex;
std::vector<Join*> liveJoins;
std::vector<bridge::HookDef> hooks;
const bridge::HookDef* originalHooks=nullptr;
Version CurrentVersion(){
    if(compat::IsLunar189())return Version::Lunar189;if(compat::IsLunar1710())return Version::Lunar1710;
    if(compat::IsLunar12111())return Version::Lunar12111;if(compat::IsEaseCation())return Version::EaseCation;
    if(compat::IsNeoForge12110())return Version::NeoForge12110;
    if(compat::IsMinecraft263())return Version::Minecraft263;
    if(!compat::UsesModernRuntime()&&!compat::IsLegacyLunar())return Version::Forge1201;
    return Version::Unsupported;
}
void Throw(JNIEnv* env,const char* reason){
    if(env->ExceptionCheck())return;auto type=env->FindClass("java/net/ConnectException");if(type)env->ThrowNew(type,reason);
}
void JNICALL End(JNIEnv* env,jclass){
    if(slot==TLS_OUT_OF_INDEXES)return;auto* join=static_cast<Join*>(TlsGetValue(slot));if(!join)return;
    TlsSetValue(slot,nullptr);
    {std::lock_guard lock(joinMutex);std::erase(liveJoins,join);}
    if(join->screen)env->DeleteGlobalRef(join->screen);delete join;
}
void BeginImpl(JNIEnv* env,jclass type,jobject thread){
    End(env,type);if(stopping.load())return;
    auto cfg=proxy::ReadConfig();if(!cfg.enabled)return;
    if(slot==TLS_OUT_OF_INDEXES){Throw(env,"Nova Proxy is unavailable");return;}
    auto join=std::make_unique<Join>();join->config=std::move(cfg);
    auto clazz=env->GetObjectClass(thread);
    const bool legacy=compat::IsLegacyLunar();const bool lunar=compat::IsLunar12111();
    const char* descriptor=legacy?"Lnet/minecraft/client/multiplayer/GuiConnecting;":lunar?"Lnet/minecraft/class_412;":"Lnet/minecraft/client/gui/screens/ConnectScreen;";
    // Find the enclosing screen by type instead of compiler-specific field names.
    jfieldID parent=nullptr,hostField=nullptr;
    jvmtiEnv* vmti=CurrentJvmti();jint count=0;jfieldID* fields=nullptr;
    if(vmti&&vmti->GetClassFields(clazz,&count,&fields)==JVMTI_ERROR_NONE){
        for(jint i=0;i<count;++i){char* signature=nullptr;
            if(vmti->GetFieldName(clazz,fields[i],nullptr,&signature,nullptr)==JVMTI_ERROR_NONE){
                if(signature&&std::string(signature)==descriptor)parent=fields[i];
                if(legacy&&signature&&std::string(signature)=="Ljava/lang/String;")hostField=fields[i];
                if(signature)vmti->Deallocate(reinterpret_cast<unsigned char*>(signature));
            }
        }if(fields)vmti->Deallocate(reinterpret_cast<unsigned char*>(fields));
    }
    if(hostField){
        auto text=static_cast<jstring>(env->GetObjectField(thread,hostField));
        if(text&&!env->ExceptionCheck()){
            const char* value=env->GetStringUTFChars(text,nullptr);
            if(value){join->requestedHost=value;env->ReleaseStringUTFChars(text,value);}env->DeleteLocalRef(text);
        }
    }
    auto screen=parent?env->GetObjectField(thread,parent):nullptr;
    if(screen&&!env->ExceptionCheck()){
        auto screenClass=env->GetObjectClass(screen);
        const char* names[]={legacy?"cancel":lunar?"field_2409":"aborted","f_95685_"};
        for(auto name:names){join->cancel=env->GetFieldID(screenClass,name,"Z");
            if(env->ExceptionCheck())env->ExceptionClear();if(join->cancel)break;
        }
        if(join->cancel)join->screen=env->NewGlobalRef(screen);
        env->DeleteLocalRef(screenClass);env->DeleteLocalRef(screen);
    }
    env->DeleteLocalRef(clazz);
    if(env->ExceptionCheck())return;
    if(!join->screen||!join->cancel){Throw(env,"Nova Proxy join/cancel adapter could not resolve");return;}
    if(!TlsSetValue(slot,join.get())){
        env->DeleteGlobalRef(join->screen);Throw(env,"Nova Proxy could not create connection scope");return;
    }
    try{std::lock_guard lock(joinMutex);liveJoins.push_back(join.get());}
    catch(...){TlsSetValue(slot,nullptr);env->DeleteGlobalRef(join->screen);throw;}
    join.release();
}
void JNICALL Begin(JNIEnv* env,jclass type,jobject thread){
    try{BeginImpl(env,type,thread);}catch(...){Throw(env,"Nova Proxy could not prepare connection scope");}
}
jobject JNICALL Connect(JNIEnv* env,jclass,jobject bootstrap,jobject address,jint port){
    auto* join=slot==TLS_OUT_OF_INDEXES?nullptr:static_cast<Join*>(TlsGetValue(slot));
    auto type=env->GetObjectClass(bootstrap);
    const auto original=env->GetMethodID(type,"connect","(Ljava/net/InetAddress;I)Lio/netty/channel/ChannelFuture;");
    env->DeleteLocalRef(type);if(!original||env->ExceptionCheck())return nullptr;
    if(!join)return env->CallObjectMethod(bootstrap,original,address,port); // status ping/local scope
    proxy::Prepared prepared;
    try{
        if(stopping.load()||IsCircuitOpen())throw std::runtime_error("Nova Proxy is stopping");
        auto ipClass=env->GetObjectClass(address);auto hostMethod=env->GetMethodID(ipClass,"getHostAddress","()Ljava/lang/String;");
        auto host=hostMethod?static_cast<jstring>(env->CallObjectMethod(address,hostMethod)):nullptr;
        if(!host||env->ExceptionCheck()){env->DeleteLocalRef(ipClass);return nullptr;}
        const char* chars=env->GetStringUTFChars(host,nullptr);if(!chars)return nullptr;
        std::string target(chars);env->ReleaseStringUTFChars(host,chars);env->DeleteLocalRef(host);
        if(proxy::FakeAddress(target)){
            std::string named;
            // Use the original name rather than performing a reverse DNS lookup
            // when the legacy connector already gave us the hostname.
            if(join->requestedHost.empty()){
                auto nameMethod=env->GetMethodID(ipClass,"getHostName","()Ljava/lang/String;");
                auto name=nameMethod?static_cast<jstring>(env->CallObjectMethod(address,nameMethod)):nullptr;
                if(name&&!env->ExceptionCheck()){
                    const char* text=env->GetStringUTFChars(name,nullptr);
                    if(text){named=text;env->ReleaseStringUTFChars(name,text);}env->DeleteLocalRef(name);
                }
                if(env->ExceptionCheck()){env->DeleteLocalRef(ipClass);return nullptr;}
            }
            target=proxy::SelectTarget(target,named,join->requestedHost);
            if(proxy::FakeAddress(target))throw std::runtime_error("Proxy cannot route this Fake-IP; server hostname is unavailable");
        }
        auto cancel=[&]{return stopping.load()||IsCircuitOpen()||env->GetBooleanField(join->screen,join->cancel)==JNI_TRUE||env->ExceptionCheck();};
        prepared=proxy::Prepare(join->config,target,port,cancel);
        auto factory=env->GetStaticMethodID(ipClass,"getByAddress","([B)Ljava/net/InetAddress;");
        auto bytes=env->NewByteArray(4);const jbyte loop[]{127,0,0,1};env->SetByteArrayRegion(bytes,0,4,loop);
        auto local=factory?env->CallStaticObjectMethod(ipClass,factory,bytes):nullptr;
        env->DeleteLocalRef(bytes);env->DeleteLocalRef(ipClass);
        if(!local||env->ExceptionCheck()){proxy::Abandon(prepared);return nullptr;}
        if(cancel()){env->DeleteLocalRef(local);proxy::Abandon(prepared);Throw(env,"Proxy connection cancelled");return nullptr;}
        auto future=env->CallObjectMethod(bootstrap,original,local,prepared.port);
        env->DeleteLocalRef(local);if(!future||env->ExceptionCheck())proxy::Abandon(prepared);
        return future;
    }catch(const std::exception& e){proxy::Abandon(prepared);proxy::ReportError(e.what());Throw(env,e.what());return nullptr;}
    catch(...){proxy::Abandon(prepared);proxy::ReportError("Proxy connection failed");Throw(env,"Nova Proxy connection failed");return nullptr;}
}
}
const bridge::HookDef* ExtendHooks(const bridge::HookDef* base,int& count){
    if(CurrentVersion()==Version::Unsupported)return base;
    if(originalHooks!=base){originalHooks=base;hooks.assign(base,base+count);
        hooks.push_back({"proxyJoinBegin","(Ljava/lang/Object;)V",reinterpret_cast<void*>(&Begin)});
        hooks.push_back({"proxyJoinEnd","()V",reinterpret_cast<void*>(&End)});
        hooks.push_back({"proxyConnect","(Ljava/lang/Object;Ljava/net/InetAddress;I)Lio/netty/channel/ChannelFuture;",reinterpret_cast<void*>(&Connect)});
    }count=int(hooks.size());return hooks.data();
}
void Initialize(){if(slot==TLS_OUT_OF_INDEXES)slot=TlsAlloc();proxy::SetSupported(slot!=TLS_OUT_OF_INDEXES&&CurrentVersion()!=Version::Unsupported);}
void Shutdown(JNIEnv* env){
    End(env,nullptr);
    // Called only after hook quiescence. Exceptional run() exits can bypass a
    // normal-return append, so drain those scopes too before unmapping.
    {std::lock_guard lock(joinMutex);for(auto* join:liveJoins){if(join->screen)env->DeleteGlobalRef(join->screen);delete join;}liveJoins.clear();}
    if(slot!=TLS_OUT_OF_INDEXES){TlsFree(slot);slot=TLS_OUT_OF_INDEXES;}
}
}
