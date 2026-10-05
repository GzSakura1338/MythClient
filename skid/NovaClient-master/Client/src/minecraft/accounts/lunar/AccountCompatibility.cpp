#include "minecraft/accounts/lunar/AccountCompatibility.h"
#include "minecraft/accounts/lunar/AccountEdits.h"
#include "minecraft/accounts/lunar/CheckedJni.h"
#include "minecraft/accounts/lunar/LunarMappings.h"
#include "core/RuntimeControl.h"

namespace nova::minecraft::accounts {
void SelectAccountBuild(JNIEnv* env){
    namespace a=nova::minecraft::accounts;
    namespace m=a::mapping;
    m::activeBuild=m::Build::Unknown;
    for(auto build:{m::Build::October2,m::Build::September30,m::Build::September27}){
        m::activeBuild=build;
        try{
            a::Jni j(env);m::ResolvedAccountEdits probe;probe.Reset(build);
            auto method=[&](const char* owner,const char* name,const char* desc,bool either=true){
                auto c=j.cls(owner);const auto n=m::Remap(name),d=m::Remap(desc);
                auto id=env->GetMethodID(c,n.c_str(),d.c_str());
                if(env->ExceptionCheck())env->ExceptionClear();
                if(!id&&either)id=env->GetStaticMethodID(c,n.c_str(),d.c_str());
                j.required(id);
            };
            // All account hooks form one optional group. Never install only a
            // subset that could leave account navigation/login half intercepted.
            for(const auto& row:probe.classes)for(int i=0;i<row.count;++i){
                const auto& r=row.redirects[i];method(row.internalName,r.method,r.methodDesc);
                method(r.calleeOwner,r.calleeName,r.calleeDesc);
            }
            method(m::Client,m::Singleton,a::Jni::returns(m::Client).c_str());
            method(m::Client,m::GetManager,a::Jni::returns(m::Manager).c_str(),false);
            method(m::Manager,m::GetAccounts,"()Ljava/util/Map;",false);
            method(m::Manager,m::GetCurrent,a::Jni::returns(m::Account).c_str(),false);
            method(m::Manager,m::SetSelected,"(Ljava/lang/String;)V",false);
            method(m::Manager,m::GetModel,a::Jni::returns(m::Model).c_str(),false);
            method(m::Model,m::ModelMap,"()Ljava/util/Map;",false);
            method(m::Model,"IRRICCROHCCHROCCCICOICOCOCIORI","(Ljava/lang/String;Ljava/lang/Object;)V",false);
            method(m::Account,m::GetProfile,a::Jni::returns(m::Profile).c_str(),false);
            for(auto name:{"isLoading","isInvalid","RHHRROIOICRORRHHHHOROOHCHHHOCO"})method(m::Account,name,"()Z",false);
            for(auto name:{"CHRCCICICHHHHHRRRRCICCIOHRCOCR","IHCHRHOIIHCIICHRROHOCROHCOIIOH"})method(m::Manager,name,"(Z)V",false);
            method(m::Manager,"HCOOHCRIIRHROCROHROOOOORCRORIR","(Ljava/lang/String;Z)V",false);
            method(m::Account,"CRHHRIOHIOCICIHICRHOCRCHIOCCCR","(Z)V",false);
            method(m::Client,m::SessionChanged,("("+a::Jni::descriptor(m::SessionBridge)+")V").c_str(),false);
            LogTo("Lunar accounts: verified mapping build=%s",build==m::Build::October2?"2026-10-02":build==m::Build::September30?"2026-09-30":"2026-09-27");
            return;
        }catch(...){if(env->ExceptionCheck())env->ExceptionClear();}
    }
    m::activeBuild=m::Build::Unknown;
    LogTo("Lunar accounts: unknown account mapping; account integration disabled, core event hooks retained");
}

namespace {
mapping::ResolvedAccountEdits legacyAccountEdits;
std::vector<hook::ClassEdits> legacyEdits;
}
void SelectLegacyAccountEdits(JNIEnv* env) {
    SelectAccountBuild(env);
    // Preserve the selected 1.8.9 core rules and append only verified accounts.
    int count=0;const auto* base=hook::BaseEdits(count);
    legacyEdits.assign(base,base+count);
    legacyAccountEdits.Reset(mapping::activeBuild.load());
    legacyEdits.insert(legacyEdits.end(),legacyAccountEdits.classes.begin(),legacyAccountEdits.classes.end());
    hook::SelectCustomEdits(legacyEdits.data(),static_cast<int>(legacyEdits.size()));
}
}
