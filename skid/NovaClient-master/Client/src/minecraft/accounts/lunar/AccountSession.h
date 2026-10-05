#pragma once
#include "minecraft/accounts/lunar/CheckedJni.h"
#include "minecraft/accounts/AccountValidation.h"

namespace nova::minecraft::accounts::session {
inline bool SupportedVersion(){return compat::IsLunar189()||compat::IsLunar12111();}
inline jobject User(Jni& j,jobject mc){
    return j.required(compat::IsLunar189()?j.get(mc,"session","Lnet/minecraft/util/Session;"):
        j.get(mc,"field_1726","Lnet/minecraft/class_320;"));
}
inline std::string Name(Jni& j,jobject user){
    return j.text(j.call(user,compat::IsLunar189()?"getUsername":"method_1676","()Ljava/lang/String;"));
}
inline std::string Uuid(Jni& j,jobject user){
    if(compat::IsLunar189())return NormalizeUuid(j.text(j.call(user,"getPlayerID","()Ljava/lang/String;")));
    return NormalizeUuid(j.text(j.call(j.call(user,"method_44717","()Ljava/util/UUID;"),"toString","()Ljava/lang/String;")));
}
}
