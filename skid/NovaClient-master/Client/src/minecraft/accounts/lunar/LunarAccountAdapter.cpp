#include "minecraft/accounts/lunar/LunarAccountAdapter.h"
#include "minecraft/accounts/lunar/LunarMappings.h"
#include "minecraft/accounts/lunar/CheckedJni.h"
#include "minecraft/accounts/lunar/AccountSession.h"
#include "minecraft/accounts/AccountValidation.h"
#include "minecraft/accounts/TokenAuth.h"
#include <array>
#include <utility>

namespace nova::minecraft::accounts {
namespace {
namespace m=mapping;
constexpr const char* UserApi="com.mojang.authlib.minecraft.UserApiService";
constexpr const char* ApiSig="Lcom/mojang/authlib/minecraft/UserApiService;";
constexpr const char* PropertiesSig="Lcom/mojang/authlib/minecraft/UserApiService$UserProperties;";
jobject Client(Jni& j){return j.required(j.stat(m::Client,m::Singleton,Jni::returns(m::Client).c_str()));}
jobject Manager(Jni& j){return j.required(j.call(Client(j),m::GetManager,Jni::returns(m::Manager).c_str()));}
jobject Minecraft(Jni& j){return j.required(compat::Minecraft(j.e));}
using session::User;
using session::Uuid;
bool InWorld(Jni& j,jobject mc) {
    if(compat::IsLunar189()) {
        if(j.get(mc,"theWorld","Lnet/minecraft/client/multiplayer/WorldClient;")||
           j.get(mc,"thePlayer","Lnet/minecraft/client/entity/EntityPlayerSP;")||
           j.call(mc,"getNetHandler","()Lnet/minecraft/client/network/NetHandlerPlayClient;"))return true;
        // Minecraft's constructor creates an IntegratedServer placeholder even
        // at the title screen. Like loadWorld(null), distinguish it from a real
        // server by its save directory, including startup/shutdown transitions.
        auto server=j.get(mc,"theIntegratedServer","Lnet/minecraft/server/integrated/IntegratedServer;");
        if(server&&j.flag(server,"isAnvilFileSet"))return true;
        // myNetworkManager can retain a closed integrated-server connection.
        auto connection=j.get(mc,"myNetworkManager","Lnet/minecraft/network/NetworkManager;");
        if(connection&&j.flag(connection,"isChannelOpen"))return true;
        auto screen=j.get(mc,"currentScreen","Lnet/minecraft/client/gui/GuiScreen;");
        return !screen||j.e->IsInstanceOf(screen,j.cls("net.minecraft.client.multiplayer.GuiConnecting"))||
            j.e->IsInstanceOf(screen,j.cls("net.minecraft.client.gui.GuiDownloadTerrain"));
    }
    if(j.get(mc,"field_1687","Lnet/minecraft/class_638;")||j.get(mc,"field_1724","Lnet/minecraft/class_746;")||
       j.get(mc,"field_1766","Lnet/minecraft/class_1132;")||j.get(mc,"field_1746","Lnet/minecraft/class_2535;")||
       j.call(mc,"method_1562","()Lnet/minecraft/class_634;"))return true;
    // Lunar replaces the vanilla title screen. Class identity must not turn a
    // disconnected custom menu into a world. Still block the connection worker
    // before Minecraft.pendingConnection becomes available.
    auto screen=j.get(mc,"field_1755","Lnet/minecraft/class_437;");
    return !screen || j.e->IsInstanceOf(screen,j.cls("net.minecraft.class_412")) ||
        j.e->IsInstanceOf(screen,j.cls("net.minecraft.class_435")) ||
        j.e->IsInstanceOf(screen,j.cls("net.minecraft.class_424"));
}
bool Busy(Jni& j,jobject manager) {
    auto model=j.call(manager,m::GetModel,Jni::returns(m::Model).c_str());
    auto map=j.call(model,m::ModelMap,"()Ljava/util/Map;");
    for(auto key:{"addingAccount","loggingIn"}) {
        auto value=j.call(map,"get","(Ljava/lang/Object;)Ljava/lang/Object;",j.str(key));
        if(value && j.flag(value,"booleanValue"))return true;
    }
    auto values=j.call(j.call(manager,m::GetAccounts,"()Ljava/util/Map;"),"values","()Ljava/util/Collection;");
    auto it=j.call(values,"iterator","()Ljava/util/Iterator;");
    for(int n=0;n<128&&j.flag(it,"hasNext");++n)if(j.flag(j.call(it,"next","()Ljava/lang/Object;"),"isLoading"))return true;
    return false;
}
jobject FindAccount(Jni& j,jobject manager,const std::string& id) {
    auto values=j.call(j.call(manager,m::GetAccounts,"()Ljava/util/Map;"),"values","()Ljava/util/Collection;");
    auto it=j.call(values,"iterator","()Ljava/util/Iterator;");
    for(int n=0;n<128&&j.flag(it,"hasNext");++n){auto a=j.call(it,"next","()Ljava/lang/Object;");auto p=j.call(a,m::GetProfile,Jni::returns(m::Profile).c_str());
        if(p && "lunar:"+NormalizeUuid(j.text(j.call(p,"getId","()Ljava/lang/String;")))==id)return a;}
    return nullptr;
}
jobject Complete(Jni& j,jobject value){return j.stat("java.util.concurrent.CompletableFuture","completedFuture","(Ljava/lang/Object;)Ljava/util/concurrent/CompletableFuture;",value);}
bool CommitLegacy(Jni& j,jobject mc,jobject manager,Prepared& ready) {
    if(!ready.user||!ready.properties)throw JniFailure{};
    auto* env=j.e;
    const auto userField=j.field(mc,"session","Lnet/minecraft/util/Session;");
    const auto propertiesField=j.field(mc,"profileProperties","Lcom/mojang/authlib/properties/PropertyMap;");
    auto oldUser=User(j,mc);auto oldProperties=env->GetObjectField(mc,propertiesField);j.check();
    auto current=j.call(manager,m::GetCurrent,Jni::returns(m::Account).c_str());
    auto oldSelected=current?j.call(current,"getUsername","()Ljava/lang/String;"):nullptr;
    auto selected=ready.login.account.source==nova::accounts::Source::Lunar?j.str(ready.login.account.name):nullptr;
    auto selection=j.method(manager,m::SetSelected,"(Ljava/lang/String;)V");
    auto client=Client(j);auto changed=j.method(client,m::SessionChanged,("("+Jni::descriptor(m::SessionBridge)+")V").c_str());
    auto model=j.call(manager,m::GetModel,Jni::returns(m::Model).c_str());
    auto modelSet=j.method(model,"IRRICCROHCCHROCCCICOICOCOCIORI","(Ljava/lang/String;Ljava/lang/Object;)V");
    auto selectedKey=j.str("selectedAccount");auto modelMap=j.call(model,m::ModelMap,"()Ljava/util/Map;");
    auto oldModelSelection=j.call(modelMap,"get","(Ljava/lang/Object;)Ljava/lang/Object;",selectedKey);
    auto sentry=j.cls("io.sentry.Sentry");auto setSentry=j.required(env->GetStaticMethodID(sentry,"setUser","(Lio/sentry/protocol/User;)V"));
    jobject sentryUser=nullptr;
    if(selected){sentryUser=j.make("io.sentry.protocol.User","()V");j.run(sentryUser,"setUsername","(Ljava/lang/String;)V",selected);j.run(sentryUser,"setId","(Ljava/lang/String;)V",j.str(DashedUuid(ready.login.account.uuid)));}
    try {
        env->CallVoidMethod(manager,selection,selected);j.check();
        if(!selected){env->CallVoidMethod(model,modelSet,selectedKey,nullptr);j.check();}
        // Same state transition as Lunar 1.8.9 bridge$setSession. Publish an
        // empty cache so the old cache remains available for exact rollback;
        // notify Lunar only after both session and cache are committed.
        env->SetObjectField(mc,userField,ready.user);j.check();
        env->SetObjectField(mc,propertiesField,ready.properties);j.check();
        if(Uuid(j,User(j,mc))!=ready.login.account.uuid||session::Name(j,User(j,mc))!=ready.login.account.name)throw JniFailure{};
    }catch(...) {
        env->SetObjectField(mc,userField,oldUser);if(env->ExceptionCheck())env->ExceptionClear();
        env->SetObjectField(mc,propertiesField,oldProperties);if(env->ExceptionCheck())env->ExceptionClear();
        env->CallVoidMethod(manager,selection,oldSelected);if(env->ExceptionCheck())env->ExceptionClear();
        env->CallVoidMethod(model,modelSet,selectedKey,oldModelSelection);if(env->ExceptionCheck())env->ExceptionClear();
        throw;
    }
    env->CallStaticVoidMethod(sentry,setSentry,sentryUser);if(env->ExceptionCheck())env->ExceptionClear();
    env->CallVoidMethod(client,changed,oldUser);if(env->ExceptionCheck())env->ExceptionClear();
    return true;
}
}

RuntimeView Inspect(JNIEnv* env) {
    RuntimeView out;
    if(!session::SupportedVersion()){out.snapshot.status="Account switching supports Lunar 1.8.9 and 1.21.11.";return out;}
    if(!m::Supported()){out.snapshot.status="This Lunar account page needs a mapping update.";return out;}
    Jni j(env);auto mc=Minecraft(j);auto manager=Manager(j);auto user=User(j,mc);
    out.snapshot.currentName=session::Name(j,user);out.currentUuid=Uuid(j,user);
    out.snapshot.inWorld=InWorld(j,mc);out.lunarBusy=Busy(j,manager);
    auto values=j.call(j.call(manager,m::GetAccounts,"()Ljava/util/Map;"),"values","()Ljava/util/Collection;");
    auto it=j.call(values,"iterator","()Ljava/util/Iterator;");
    for(int n=0;n<128&&j.flag(it,"hasNext");++n) {
        auto a=j.call(it,"next","()Ljava/lang/Object;");auto profile=j.call(a,m::GetProfile,Jni::returns(m::Profile).c_str());if(!profile)continue;
        nova::accounts::Account row;row.uuid=NormalizeUuid(j.text(j.call(profile,"getId","()Ljava/lang/String;")));if(row.uuid.empty())continue;
        row.id="lunar:"+row.uuid;row.name=j.text(j.call(a,"getUsername","()Ljava/lang/String;"));row.source=nova::accounts::Source::Lunar;
        row.current=row.uuid==out.currentUuid;row.invalid=j.flag(a,"isInvalid")||!j.flag(a,"RHHRROIOICRORRHHHHOROOHCHHHOCO");out.snapshot.rows.push_back(std::move(row));
    }
    out.snapshot.supported=true;return out;
}
Login ReadLunarLogin(JNIEnv* env,const std::string& id,const std::string& expected) {
    Jni j(env);auto a=j.required(FindAccount(j,Manager(j),id));Login login;login.expectedUuid=expected;
    login.account={id,j.text(j.call(a,"getUsername","()Ljava/lang/String;")),id.substr(6),nova::accounts::Source::Lunar};
    login.token=j.text(j.call(a,"getAccessToken","()Ljava/lang/String;"));return login;
}
jobject CaptureSession(JNIEnv* env){Jni j(env);return j.global(User(j,Minecraft(j)));}
void Prepared::Release(JNIEnv* env){for(auto* ref:{&user,&api,&properties,&profile,&expectedUser})if(*ref){env->DeleteGlobalRef(*ref);*ref=nullptr;}for(auto* secret:{&login.token,&login.refresh})if(!secret->empty()){SecureZeroMemory(secret->data(),secret->size());secret->clear();}}

void Prepare(Prepared& out,const std::atomic<bool>& cancelled) {
    try {
        const bool offline=out.login.account.source==nova::accounts::Source::Offline;
        if(!offline){auto profile=VerifyToken(out.login.token,cancelled);if(!profile.error.empty())throw profile.error;
            if(!out.login.account.uuid.empty()&&out.login.account.uuid!=profile.uuid)throw std::string("The account identity changed. Sign in again.");
            out.login.account.name=profile.name;out.login.account.uuid=profile.uuid;}
        if(cancelled.load())throw std::string("Cancelled.");
        JniAttach attach;if(!attach)throw JniFailure{};Jni j(attach.env);
        jobject uuid;
        if(offline){const std::string name="OfflinePlayer:"+out.login.account.name;auto bytes=j.required(j.e->NewByteArray(jsize(name.size())));j.e->SetByteArrayRegion(bytes,0,jsize(name.size()),reinterpret_cast<const jbyte*>(name.data()));j.check();
            uuid=j.stat("java.util.UUID","nameUUIDFromBytes","([B)Ljava/util/UUID;",bytes);out.login.account.uuid=NormalizeUuid(j.text(j.call(uuid,"toString","()Ljava/lang/String;")));}
        else uuid=j.stat("java.util.UUID","fromString","(Ljava/lang/String;)Ljava/util/UUID;",j.str(DashedUuid(out.login.account.uuid)));
        if(compat::IsLunar189()) {
            // 1.8.9 has no UserApiService/profile-key/telemetry session objects.
            // Its Session.Type only accepts legacy/mojang, including MS tokens.
            auto user=j.make("net.minecraft.util.Session","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
                j.str(out.login.account.name),j.str(out.login.account.uuid),j.str(offline?"":out.login.token),j.str(offline?"legacy":"mojang"));
            auto properties=j.make("com.mojang.authlib.properties.PropertyMap","()V");
            if(cancelled.load())throw std::string("Cancelled.");
            out.user=j.global(user);out.properties=j.global(properties);
            if(out.login.account.id.empty())out.login.account.id=(offline?"offline:":out.login.account.source==nova::accounts::Source::Microsoft?"microsoft:":"token:")+out.login.account.uuid;
            return;
        }
        auto empty=j.stat("java.util.Optional","empty","()Ljava/util/Optional;");
        auto user=j.make("net.minecraft.class_320","(Ljava/lang/String;Ljava/util/UUID;Ljava/lang/String;Ljava/util/Optional;Ljava/util/Optional;)V",j.str(out.login.account.name),uuid,j.str(offline?"":out.login.token),empty,empty);
        jobject api,props,profile=nullptr;
        if(offline){api=j.constant(UserApi,"OFFLINE",ApiSig);props=j.constant(UserApi,"OFFLINE_PROPERTIES",PropertiesSig);}
        else {
            // Authlib network work remains off the game/render threads.
            auto proxy=j.constant("java.net.Proxy","NO_PROXY","Ljava/net/Proxy;");
            auto auth=j.make("com.mojang.authlib.yggdrasil.YggdrasilAuthenticationService","(Ljava/net/Proxy;)V",proxy);
            api=j.call(auth,"createUserApiService","(Ljava/lang/String;)Lcom/mojang/authlib/minecraft/UserApiService;",j.str(out.login.token));
            props=j.call(api,"fetchProperties","()Lcom/mojang/authlib/minecraft/UserApiService$UserProperties;");
            if(cancelled.load())throw std::string("Cancelled.");
            auto service=j.call(auth,"createMinecraftSessionService","()Lcom/mojang/authlib/minecraft/MinecraftSessionService;");
            profile=j.call(service,"fetchProfile","(Ljava/util/UUID;Z)Lcom/mojang/authlib/yggdrasil/ProfileResult;",uuid,JNI_TRUE);
        }
        if(cancelled.load())throw std::string("Cancelled.");
        out.user=j.global(user);out.api=j.global(j.required(api));out.properties=j.global(j.required(props));out.profile=j.global(profile);
        if(out.login.account.id.empty())out.login.account.id=(offline?"offline:":out.login.account.source==nova::accounts::Source::Microsoft?"microsoft:":"token:")+out.login.account.uuid;
    }catch(const std::string& error){out.error=error;}catch(...){out.error="Could not prepare the account session. Check connectivity and client compatibility.";}
}

bool Commit(JNIEnv* env,Prepared& ready) {
    Jni j(env);auto mc=Minecraft(j);auto manager=Manager(j);
    if(InWorld(j,mc)||Busy(j,manager)||!ready.expectedUser||!env->IsSameObject(User(j,mc),ready.expectedUser)||Uuid(j,User(j,mc))!=ready.login.expectedUuid)return false;
    if(compat::IsLunar189())return CommitLegacy(j,mc,manager,ready);
    if(!ready.user||!ready.api||!ready.properties)throw JniFailure{};
    // Resolve and build every dependent object before changing the current user.
    auto file=j.get(mc,"field_1697","Ljava/io/File;");auto path=j.call(file,"toPath","()Ljava/nio/file/Path;");
    auto keys=j.stat("net.minecraft.class_7853","method_46532","(Lcom/mojang/authlib/minecraft/UserApiService;Lnet/minecraft/class_320;Ljava/nio/file/Path;)Lnet/minecraft/class_7853;",ready.api,ready.user,path);
    auto social=j.make("net.minecraft.class_5520","(Lnet/minecraft/class_310;Lcom/mojang/authlib/minecraft/UserApiService;)V",mc,ready.api);
    auto environment=j.stat("net.minecraft.class_7569","method_44586","()Lnet/minecraft/class_7569;");
    auto reporting=j.stat("net.minecraft.class_7574","method_44599","(Lnet/minecraft/class_7569;Lcom/mojang/authlib/minecraft/UserApiService;)Lnet/minecraft/class_7574;",environment,ready.api);
    auto realms=j.make("net.minecraft.class_4341","(Ljava/lang/String;Ljava/lang/String;Lnet/minecraft/class_310;)V",j.call(ready.user,"method_1675","()Ljava/lang/String;"),j.str(ready.login.account.name),mc);
    auto fetcher=j.make("net.minecraft.class_7578","(Lnet/minecraft/class_4341;)V",realms);
    auto telemetry=j.make("net.minecraft.class_6628","(Lnet/minecraft/class_310;Lcom/mojang/authlib/minecraft/UserApiService;Lnet/minecraft/class_320;)V",mc,ready.api,ready.user);
    const auto close=j.method(telemetry,"close","()V");
    struct TelemetryGuard {
        JNIEnv* env;jobject object;jmethodID close;bool published=false;
        ~TelemetryGuard(){if(!published){env->CallVoidMethod(object,close);if(env->ExceptionCheck())env->ExceptionClear();}}
    } telemetryGuard{env,telemetry,close};
    struct Slot { const char* name;const char* sig;jobject value;jfieldID field{};jobject old{}; };
    std::array<Slot,9> slots{{
        {"field_1726","Lnet/minecraft/class_320;",ready.user},
        {"field_26902",ApiSig,ready.api},
        {"field_47680","Ljava/util/concurrent/CompletableFuture;",Complete(j,ready.properties)},
        {"field_45899","Ljava/util/concurrent/CompletableFuture;",Complete(j,ready.profile)},
        {"field_39068","Lnet/minecraft/class_7853;",keys},
        {"field_26842","Lnet/minecraft/class_5520;",social},
        {"field_39492","Lnet/minecraft/class_7574;",reporting},
        {"field_39718","Lnet/minecraft/class_7578;",fetcher},
        {"field_41331","Lnet/minecraft/class_6628;",telemetry}
    }};
    for(auto& s:slots){s.field=j.field(mc,s.name,s.sig);s.old=j.e->GetObjectField(mc,s.field);j.check();}
    auto realmsClass=j.cls("net.minecraft.class_4341");auto realmsField=j.required(j.e->GetStaticFieldID(realmsClass,"field_57916","Lnet/minecraft/class_4341;"));
    auto oldRealms=j.e->GetStaticObjectField(realmsClass,realmsField);j.check();
    auto splash=j.get(mc,"field_17763","Lnet/minecraft/class_4008;");auto splashField=j.field(splash,"field_18934","Lnet/minecraft/class_320;");auto oldSplash=j.e->GetObjectField(splash,splashField);j.check();
    auto current=j.call(manager,m::GetCurrent,Jni::returns(m::Account).c_str());auto oldSelected=current?j.call(current,"getUsername","()Ljava/lang/String;"):nullptr;
    auto selected=ready.login.account.source==nova::accounts::Source::Lunar?j.str(ready.login.account.name):nullptr;
    auto selection=j.method(manager,m::SetSelected,"(Ljava/lang/String;)V");
    auto client=Client(j);auto changed=j.method(client,m::SessionChanged,("("+Jni::descriptor(m::SessionBridge)+")V").c_str());
    auto model=j.call(manager,m::GetModel,Jni::returns(m::Model).c_str());
    auto modelSet=j.method(model,"IRRICCROHCCHROCCCICOICOCOCIORI","(Ljava/lang/String;Ljava/lang/Object;)V");
    auto selectedKey=j.str("selectedAccount");
    auto sentry=j.cls("io.sentry.Sentry");auto setSentry=j.required(env->GetStaticMethodID(sentry,"setUser","(Lio/sentry/protocol/User;)V"));
    jobject sentryUser=nullptr;
    if(selected){sentryUser=j.make("io.sentry.protocol.User","()V");j.run(sentryUser,"setUsername","(Ljava/lang/String;)V",selected);j.run(sentryUser,"setId","(Ljava/lang/String;)V",j.str(DashedUuid(ready.login.account.uuid)));}
    try {
        // Lunar refresh must not retain the preceding Microsoft identity when
        // entering a local offline/token session. Its refresh loop handles null.
        j.e->CallVoidMethod(manager,selection,selected);j.check();
        if(!selected){env->CallVoidMethod(model,modelSet,selectedKey,nullptr);j.check();}
        for(auto& s:slots){j.e->SetObjectField(mc,s.field,s.value);j.check();}
        j.e->SetObjectField(splash,splashField,ready.user);j.check();
        j.e->SetStaticObjectField(realmsClass,realmsField,realms);j.check();
        if(Uuid(j,User(j,mc))!=ready.login.account.uuid)throw JniFailure{};
    }catch(...) {
        // Old telemetry is still open. Restoring these exact references is safe.
        for(auto& s:slots){j.e->SetObjectField(mc,s.field,s.old);if(j.e->ExceptionCheck())j.e->ExceptionClear();}
        j.e->SetObjectField(splash,splashField,oldSplash);j.e->SetStaticObjectField(realmsClass,realmsField,oldRealms);
        j.e->CallVoidMethod(manager,selection,oldSelected);if(j.e->ExceptionCheck())j.e->ExceptionClear();
        throw;
    }
    telemetryGuard.published=true;
    env->CallStaticVoidMethod(sentry,setSentry,sentryUser);if(env->ExceptionCheck())env->ExceptionClear();
    // Publication is complete. Lunar's own notification owns its Java callbacks.
    j.e->CallVoidMethod(client,changed,slots[0].old);if(j.e->ExceptionCheck())j.e->ExceptionClear();
    if(slots.back().old){j.e->CallVoidMethod(slots.back().old,close);if(j.e->ExceptionCheck())j.e->ExceptionClear();}
    return true;
}
}
