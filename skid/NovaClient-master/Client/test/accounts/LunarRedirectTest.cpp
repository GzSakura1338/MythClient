// Audits the production redirects against existing Lunar class files. No
// generated Java classes or live JVM/account operations are required.
#include "minecraft/accounts/lunar/AccountEdits.h"
#include "minecraft/accounts/lunar/LunarMappings.h"
#include "minecraft/java/api/class_edit.h"
#include "../bytecode/audit.h"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
std::filesystem::path LongPath(std::filesystem::path path){
    return std::filesystem::path(L"\\\\?\\"+std::filesystem::absolute(path).make_preferred().wstring());
}

bool HasMethod(const char* root,std::string owner,const std::string& name,const std::string& desc,int depth=0){
    if(depth>16)return false;std::replace(owner.begin(),owner.end(),'.','/');
    std::ifstream file(LongPath(std::filesystem::path(root)/(owner+".class")),std::ios::binary);
    std::vector<u1> bytes((std::istreambuf_iterator<char>(file)),{});if(bytes.size()<10)return false;
    if(ClassFileEditor(bytes.data(),bytes.size()).hasMethod(name,desc))return true;
    const u1* p=bytes.data()+8;const auto end=bytes.data()+bytes.size();audit::Pool cp;assert(audit::ParsePool(p,end,cp));
    auto word=[&](){assert(end-p>=2);auto n=audit::Be16(p);p+=2;return n;};
    word();word();auto super=word();std::vector<u2> parents;if(super)parents.push_back(super);
    auto count=word();for(int i=0;i<count;++i)parents.push_back(word());
    for(auto parent:parents){auto& raw=cp.bytes[cp.a[parent]];if(HasMethod(root,std::string(raw.begin(),raw.end()),name,desc,depth+1))return true;}
    return false;
}

int main(int argc,char** argv) {
    assert(argc==3);namespace m=nova::minecraft::accounts::mapping;
    m::activeBuild=std::string(argv[2])=="1002"?m::Build::October2:std::string(argv[2])=="0930"?m::Build::September30:m::Build::September27;
    m::ResolvedAccountEdits resolved;resolved.Reset(m::activeBuild);
    int redirects=0;
    for(const auto& edits:resolved.classes) {
        auto flat=std::string(edits.internalName);
        auto path=LongPath(std::filesystem::path(argv[1])/(flat+".class"));
        std::ifstream file(path,std::ios::binary);assert(file.good());
        std::vector<u1> bytes((std::istreambuf_iterator<char>(file)),{});
        ClassFileEditor editor(bytes.data(),bytes.size());
        assert(editor.thisClassInternalName()==edits.internalName);
        for(int i=0;i<edits.count;++i) {
            const auto& r=edits.redirects[i];
            assert(HasMethod(argv[1],r.calleeOwner,r.calleeName,r.calleeDesc));
            if(editor.findCalls(r.method,r.methodDesc,r.calleeOwner,r.calleeName,r.calleeDesc).size()!=1){
                std::printf("MISSING CALL %s.%s%s -> %s.%s%s\n",edits.internalName,r.method,r.methodDesc,r.calleeOwner,r.calleeName,r.calleeDesc);
                return 1;
            }
            assert(editor.redirectCalls(r.method,r.methodDesc,r.calleeOwner,r.calleeName,r.calleeDesc,
                "nova/NativeHooks",r.hook,r.hookDesc,r.opcode,r.limit,r.ordinal));
            ++redirects;
        }
        auto patched=editor.serialize();std::string error;
        if(!audit::AuditClass(patched,error)){std::puts(error.c_str());return 1;}
        ClassFileEditor checked(patched.data(),patched.size());
        for(int i=0;i<edits.count;++i) {
            const auto& r=edits.redirects[i];
            assert(checked.findCalls(r.method,r.methodDesc,r.calleeOwner,r.calleeName,r.calleeDesc).empty());
            assert(checked.findCalls(r.method,r.methodDesc,"nova/NativeHooks",r.hook,r.hookDesc).size()==1);
        }
    }
    auto ret=[](const char* owner){std::string name=owner;std::replace(name.begin(),name.end(),'.','/');return "()L"+name+";";};
    int members=0;
    auto member=[&](const char* owner,const char* name,std::string desc){
        const auto o=m::Remap(owner),n=m::Remap(name),d=m::Remap(desc);
        assert(m::Remap(o)==o&&m::Remap(n)==n&&m::Remap(d)==d);
        if(!HasMethod(argv[1],o,n,d)){std::printf("MISSING %s.%s%s\n",o.c_str(),n.c_str(),d.c_str());std::fflush(stdout);assert(false);}++members;
    };
    member(m::Client,m::Singleton,ret(m::Client));member(m::Client,m::GetManager,ret(m::Manager));
    member(m::Manager,m::GetAccounts,"()Ljava/util/Map;");member(m::Manager,m::GetCurrent,ret(m::Account));
    member(m::Manager,m::SetSelected,"(Ljava/lang/String;)V");member(m::Manager,m::GetModel,ret(m::Model));
    member(m::Model,m::ModelMap,"()Ljava/util/Map;");member(m::Model,"IRRICCROHCCHROCCCICOICOCOCIORI","(Ljava/lang/String;Ljava/lang/Object;)V");
    member(m::Account,m::GetProfile,ret(m::Profile));member(m::Profile,"getId","()Ljava/lang/String;");
    for(auto name:{"getUsername","getAccessToken"})member(m::Account,name,"()Ljava/lang/String;");
    for(auto name:{"isLoading","isInvalid","RHHRROIOICRORRHHHHOROOHCHHHOCO"})member(m::Account,name,"()Z");
    for(auto name:{"CHRCCICICHHHHHRRRRCICCIOHRCOCR","IHCHRHOIIHCIICHRROHOCROHCOIIOH"})member(m::Manager,name,"(Z)V");
    member(m::Manager,"HCOOHCRIIRHROCROHROOOOORCRORIR","(Ljava/lang/String;Z)V");member(m::Account,"CRHHRIOHIOCICIHICRHOCRCHIOCCCR","(Z)V");
    auto session=ret(m::SessionBridge);member(m::Client,m::SessionChanged,"("+session.substr(2)+")V");
    if(std::filesystem::exists(LongPath(std::filesystem::path(argv[1])/"net/minecraft/util/Session.class"))) {
        member("net.minecraft.util.Session","<init>","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
        for(auto name:{"getPlayerID","getUsername","getToken"})member("net.minecraft.util.Session",name,"()Ljava/lang/String;");
        member("net.minecraft.client.Minecraft","getNetHandler","()Lnet/minecraft/client/network/NetHandlerPlayClient;");
        member("net.minecraft.network.NetworkManager","isChannelOpen","()Z");
        member("net.minecraft.server.integrated.IntegratedServer","isAnvilFileSet","()Z");
        member("net.minecraft.client.Minecraft","bridge$setSession","("+session.substr(2)+")V");
        std::ifstream file(LongPath(std::filesystem::path(argv[1])/"net/minecraft/client/Minecraft.class"),std::ios::binary);
        std::vector<u1> bytes((std::istreambuf_iterator<char>(file)),{});
        const u1* p=bytes.data()+8;const auto end=bytes.data()+bytes.size();audit::Pool cp;assert(audit::ParsePool(p,end,cp));
        auto word=[&](){assert(end-p>=2);auto n=audit::Be16(p);p+=2;return n;};
        word();word();word();auto interfaces=word();p+=interfaces*2;
        std::vector<std::pair<std::string,std::string>> fields;
        auto utf=[&](u2 id){const auto& b=cp.bytes[id];return std::string(b.begin(),b.end());};
        auto fieldCount=word();for(int i=0;i<fieldCount;++i){word();auto name=utf(word());auto desc=utf(word());fields.emplace_back(name,desc);
            auto attributes=word();for(int a=0;a<attributes;++a){word();assert(end-p>=4);auto size=audit::Be32(p);p+=4;assert(end-p>=size);p+=size;}}
        for(auto expected:std::initializer_list<std::pair<std::string,std::string>>{
            {"session","Lnet/minecraft/util/Session;"},{"profileProperties","Lcom/mojang/authlib/properties/PropertyMap;"},
            {"theWorld","Lnet/minecraft/client/multiplayer/WorldClient;"},{"thePlayer","Lnet/minecraft/client/entity/EntityPlayerSP;"},
            {"theIntegratedServer","Lnet/minecraft/server/integrated/IntegratedServer;"},{"myNetworkManager","Lnet/minecraft/network/NetworkManager;"},
            {"currentScreen","Lnet/minecraft/client/gui/GuiScreen;"}})assert(std::find(fields.begin(),fields.end(),expected)!=fields.end());
        std::puts("PASS: Lunar 1.8.9 session constructor, identity, notification and 7 session/world fields");
    }
    m::activeBuild=m::Build::Unknown;resolved.Reset(m::Build::Unknown);assert(resolved.classes.empty()&&!m::Supported());
    std::printf("PASS: %d account JNI members; unknown builds omit the complete account hook group\n",members);
    std::printf("PASS: %d Lunar account redirects, class reparse, branches and stack-map audit\n",redirects);
}
