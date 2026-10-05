#include "minecraft/compat/lunar12111/Edits.h"
#include "minecraft/compat/lunar12111/Mapping.h"
#include "minecraft/hooks/Edits.h"
#include "minecraft/accounts/lunar/AccountEdits.h"
#ifndef NOVA_TEST_LUNAR12111
#include "minecraft/accounts/lunar/AccountCompatibility.h"
#endif
#include <cstring>

namespace nova::compat::lunar12111 {
namespace {
using namespace hook;
struct Rules {
    std::vector<Redirect> redirects;
    std::vector<Insertion> insertions;
    std::vector<Guard> guards;
    std::vector<Append> appends;
};
std::vector<Rules> storage;
std::vector<ClassEdits> edits;
nova::minecraft::accounts::mapping::ResolvedAccountEdits accountEdits;
void Prepare() {
    edits.clear();storage.clear();
    int count=0;const auto* base=GlfwModernEdits(count);
    storage.resize(count);edits.reserve(count);
    for(int i=0;i<count;++i){
        auto source=base[i];auto& target=storage[i];
        auto method=[&](const char*& name,const char*& desc){
            const auto m=Translate({source.internalName,name,desc});name=m.name;desc=m.descriptor;
        };
        auto callee=[](auto& value){
            const auto m=Translate({value.calleeOwner,value.calleeName,value.calleeDesc});
            value.calleeOwner=m.owner;value.calleeName=m.name;value.calleeDesc=m.descriptor;
        };
        for(int r=0;r<source.count;++r){
            auto value=source.redirects[r];
            // Verified against Lunar's baked class files, not guessed by version.
            if(std::strcmp(source.internalName,"net/minecraft/client/Camera")==0)
                value.methodDesc="(Lnet/minecraft/world/level/Level;Lnet/minecraft/world/entity/Entity;ZZF)V";
            if(std::strcmp(source.internalName,"net/minecraft/world/entity/player/Player")==0){
                value.method="causeExtraKnockback";value.methodDesc="(Lnet/minecraft/world/entity/Entity;FLnet/minecraft/world/phys/Vec3;)V";
            }
            if(std::strcmp(source.internalName,"net/minecraft/client/multiplayer/ClientPacketListener")==0){
                value.calleeDesc="(Lnet/minecraft/world/phys/Vec3;)V";
                value.hook="velocity";value.hookDesc="(Lnet/minecraft/world/entity/Entity;Lnet/minecraft/world/phys/Vec3;)V";
            }
            method(value.method,value.methodDesc);callee(value);
            if(std::strcmp(source.internalName,"net/minecraft/client/MouseHandler")==0 &&
               std::strcmp(value.hook,"freeLookTurn")==0){
                // Runtime MixinExtras moves turn into a generated Operation bridge.
                // Match its exact callee in either layout without naming mixin suffixes.
                value.method="*";value.methodDesc="";value.limit=-1;value.ordinal=-1;
            }
            value.hookDesc=Descriptor(value.hookDesc);target.redirects.push_back(value);
        }
        for(int r=0;r<source.insertionCount;++r){
            auto value=source.insertions[r];method(value.method,value.methodDesc);callee(value);
            value.hookDesc=Descriptor(value.hookDesc);target.insertions.push_back(value);
        }
        for(int r=0;r<source.guardCount;++r){
            auto value=source.guards[r];method(value.method,value.methodDesc);
            if(std::strcmp(source.internalName,"net/minecraft/client/Minecraft")==0 &&
               std::strcmp(value.hook,"click")==0)
                // The mixin tail requires locals that do not exist at method entry.
                value.entryReturn=true;
            value.hookDesc=Descriptor(value.hookDesc);target.guards.push_back(value);
        }
        for(int r=0;r<source.appendCount;++r){
            auto value=source.appends[r];
            if(std::strcmp(source.internalName,"net/minecraft/client/renderer/LevelRenderer")==0)
                // 1.21.11 adds a culling projection after the world projection.
                value.methodDesc="(Lcom/mojang/blaze3d/resource/GraphicsResourceAllocator;Lnet/minecraft/client/DeltaTracker;ZLnet/minecraft/client/Camera;Lorg/joml/Matrix4f;Lorg/joml/Matrix4f;Lorg/joml/Matrix4f;Lcom/mojang/blaze3d/buffers/GpuBufferSlice;Lorg/joml/Vector4f;Z)V";
            method(value.method,value.methodDesc);
            if(std::strcmp(source.internalName,"net/minecraft/client/player/KeyboardInput")==0 &&
               std::strcmp(value.hook,"input")==0)
                // Rewind may replace input and return early; publish that input too.
                value.allVoidReturns=true;
            value.hookDesc=Descriptor(value.hookDesc);target.appends.push_back(value);
        }
        source.internalName=ClassName(source.internalName);
        source.redirects=target.redirects.data();source.insertions=target.insertions.data();
        source.guards=target.guards.data();source.appends=target.appends.data();edits.push_back(source);
    }
    accountEdits.Reset(nova::minecraft::accounts::mapping::activeBuild.load());
    for(const auto& account:accountEdits.classes)edits.push_back(account);
}
}
void SelectEdits(JNIEnv* env){
#ifndef NOVA_TEST_LUNAR12111
    if(env)nova::minecraft::accounts::SelectAccountBuild(env);
#else
    (void)env;
#endif
    Prepare();hook::SelectCustomEdits(edits.data(),static_cast<int>(edits.size()));
}
}
