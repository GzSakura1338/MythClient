#include "minecraft/accounts/lunar/AccountHooks.h"
#include "minecraft/accounts/lunar/CheckedJni.h"
#include "minecraft/accounts/lunar/AccountSession.h"
#include "minecraft/accounts/lunar/LunarMappings.h"
#include "minecraft/accounts/AccountValidation.h"
#include "minecraft/accounts/AccountNavigation.h"
#include "contracts/AccountService.h"

namespace nova::minecraft::accounts {
namespace {
jobject Manager(Jni& j){auto client=j.stat(mapping::Client,mapping::Singleton,Jni::returns(mapping::Client).c_str());return j.call(client,mapping::GetManager,Jni::returns(mapping::Manager).c_str());}
bool CurrentSessionCanOpen(Jni& j,std::string_view destination) {
    if(IsCircuitOpen())return false;
    auto mc=j.required(compat::Minecraft(j.e));
    auto user=session::User(j,mc);
    const auto uuid=session::Uuid(j,user);
    const auto name=session::Name(j,user);
    return CanOpenGameMenu(destination,nova::accounts::Read(),uuid,name);
}
}
jboolean JNICALL ScreenRequiresLogin(JNIEnv* env,jclass,jobject screen) {
    try {
        Jni j(env);
        const bool original=j.flag(screen,"isRequiresAuthentication");
        if(!original||IsCircuitOpen())return original?JNI_TRUE:JNI_FALSE;
        const auto destination=j.text(j.call(screen,"name","()Ljava/lang/String;"));
        if(destination!="SINGLEPLAYER"&&destination!="MULTIPLAYER")return JNI_TRUE;
        return CurrentSessionCanOpen(j,destination)?JNI_FALSE:JNI_TRUE;
    }catch(...){if(env->ExceptionCheck())env->ExceptionClear();return JNI_TRUE;}
}
jboolean JNICALL CanOpenMultiplayer(JNIEnv* env,jclass,jobject manager) {
    try {
        Jni j(env);
        // Preserve original behaviour for Lunar-owned sessions. The exception
        // applies only to opening this screen with a matching Nova session.
        if(j.flag(manager,"OHIICIRIICRCIRCCIORRRCHOIICRIC"))return JNI_TRUE;
        return CurrentSessionCanOpen(j,"MULTIPLAYER")?JNI_TRUE:JNI_FALSE;
    }catch(...){if(env->ExceptionCheck())env->ExceptionClear();return JNI_FALSE;}
}
// Replace the two IPC dispatch sites, not the Java callback with a native
// lambda. Nova owns the request and UI completion, so no callback outlives DLL
// unload or reloads stale launcher_accounts.json over the new session.
void JNICALL BrowserAddAccount(JNIEnv* env,jclass,jobject) {
    try {
        Jni j(env);j.run(Manager(j),"CHRCCICICHHHHHRRRRCICCIOHRCOCR","(Z)V",JNI_FALSE);
        if(IsCircuitOpen())return;
        nova::accounts::RequestOpen();nova::accounts::SubmitNative(nova::accounts::Action::Microsoft);
    }catch(...){if(env->ExceptionCheck())env->ExceptionClear();}
}
jobject JNICALL BrowserSelectAccount(JNIEnv* env,jclass,jstring id) {
    try {
        Jni j(env);auto manager=Manager(j);const auto uuid=NormalizeUuid(j.text(id));
        j.run(manager,"HCOOHCRIIRHROCROHROOOOORCRORIR","(Ljava/lang/String;Z)V",id,JNI_FALSE);
        j.run(manager,"IHCHRHOIIHCIICHRROHOCROHCOIIOH","(Z)V",JNI_FALSE);
        if(!IsCircuitOpen()&&!uuid.empty()) {
            nova::accounts::RequestOpen();nova::accounts::SubmitNative(nova::accounts::Action::Select,"lunar:"+uuid);
        }
    }catch(...){if(env->ExceptionCheck())env->ExceptionClear();}
    // Make Lunar's handler take its existing early return, preventing its
    // launcher callback from reloading the old session after Nova signs in.
    auto optional=env->FindClass("java/util/Optional");if(!optional)return nullptr;
    auto empty=env->GetStaticMethodID(optional,"empty","()Ljava/util/Optional;");
    return empty?env->CallStaticObjectMethod(optional,empty):nullptr;
}
void JNICALL BrowserRefreshAccount(JNIEnv* env,jclass,jobject account,jobject) {
    try {
        Jni j(env);auto manager=Manager(j);
        auto profile=j.call(account,mapping::GetProfile,Jni::returns(mapping::Profile).c_str());
        auto uuid=NormalizeUuid(j.text(j.call(profile,"getId","()Ljava/lang/String;")));
        j.run(account,"CRHHRIOHIOCICIHICRHOCRCHIOCCCR","(Z)V",JNI_FALSE);
        j.run(manager,"IHCHRHOIIHCIICHRROHOCROHCOIIOH","(Z)V",JNI_FALSE);
        if(IsCircuitOpen()||uuid.empty()||nova::accounts::Read().inWorld)return;
        nova::accounts::RequestOpen();nova::accounts::SubmitNative(nova::accounts::Action::Select,"lunar:"+uuid);
    }catch(...){if(env->ExceptionCheck())env->ExceptionClear();}
}
}
