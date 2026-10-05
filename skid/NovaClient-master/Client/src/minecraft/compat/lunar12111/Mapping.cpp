#include "minecraft/compat/lunar12111/Mapping.h"
#include <algorithm>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace nova::compat::lunar12111 {
namespace {
struct Type { const char* from; const char* to; };
constexpr Type types[]{
#define NOVA_LUNAR12111_CLASS(from,to) {from,to},
#include "minecraft/compat/mappings/Lunar12111.inc"
#undef NOVA_LUNAR12111_CLASS
};
struct Binding { Member from; const char* to; };
constexpr Binding bindings[]{
#define NOVA_LUNAR12111_MEMBER(owner,name,desc,to) {{owner,name,desc},to},
#include "minecraft/compat/mappings/Lunar12111.inc"
#undef NOVA_LUNAR12111_MEMBER
};
std::string Key(Member m) { return std::string(m.owner)+"|"+m.name+"|"+m.descriptor; }
const auto classMap=[] {
    std::unordered_map<std::string_view,const char*> result;
    for(auto& t:types) result.emplace(t.from,t.to);
    return result;
}();
const auto memberMap=[] {
    std::unordered_map<std::string,const char*> result;
    for(auto& b:bindings) result.emplace(Key(b.from),b.to);
    return result;
}();
std::mutex mutex;
std::unordered_set<std::string> strings;
const char* Intern(std::string value) { std::lock_guard lock(mutex);return strings.insert(std::move(value)).first->c_str(); }
// 26.3 introduced wrapper types / renamed entry points. These aliases preserve
// the JNI argument ABI; changes to arguments belong in the runtime adapter.
const char* NamedClass(const char* name) {
    const std::string_view n(name);
    if(n=="net.minecraft.world.inventory.ContainerInput") return "net.minecraft.world.inventory.ClickType";
    if(n=="net.minecraft.world.entity.monster.cubemob.Slime") return "net.minecraft.world.entity.monster.Slime";
    if(n=="net.minecraft.core.TypedInstance"||n=="net.minecraft.world.item.ItemInstance") return "net.minecraft.world.item.ItemStack";
    return name;
}
std::string NamedDescriptor(const char* desc) {
    std::string result(desc);
    for(size_t p=0;(p=result.find('L',p))!=std::string::npos;) {
        const auto end=result.find(';',p);if(end==std::string::npos)break;
        auto name=result.substr(p+1,end-p-1);std::replace(name.begin(),name.end(),'/','.');
        std::string mapped=NamedClass(name.c_str());std::replace(mapped.begin(),mapped.end(),'.','/');
        result.replace(p+1,end-p-1,mapped);p+=mapped.size()+2;
    }
    return result;
}
}
const char* ClassName(const char* name) {
    if(!name)return name;
    const bool internal=std::string_view(name).find('/')!=std::string_view::npos;
    std::string dot(name);if(internal)std::replace(dot.begin(),dot.end(),'/','.');
    const auto named=NamedClass(dot.c_str());
    const auto it=classMap.find(named);
    if(it==classMap.end())return name;
    if(!internal)return it->second;
    std::string mapped(it->second);std::replace(mapped.begin(),mapped.end(),'.','/');return Intern(std::move(mapped));
}
const char* Descriptor(const char* descriptor) {
    if(!descriptor)return descriptor;
    std::string result=NamedDescriptor(descriptor);
    for(size_t p=0;(p=result.find('L',p))!=std::string::npos;) {
        const auto end=result.find(';',p);if(end==std::string::npos)break;
        auto name=result.substr(p+1,end-p-1);std::string mapped=ClassName(name.c_str());
        result.replace(p+1,end-p-1,mapped);p+=mapped.size()+2;
    }
    return Intern(std::move(result));
}
Member Translate(Member m) {
    if(!m.owner||!m.name||!m.descriptor)return m;
    std::string owner(m.owner);std::replace(owner.begin(),owner.end(),'/','.');
    owner=NamedClass(owner.c_str());
    auto desc=NamedDescriptor(m.descriptor);
    std::string_view name(m.name);
    if(owner=="net.minecraft.client.Camera"){
        if(name=="getPosition")m.name="position";
        if(name=="getYRot")m.name="yRot";
        if(name=="getXRot")m.name="xRot";
    }
    if(owner=="net.minecraft.client.multiplayer.MultiPlayerGameMode"&&name=="handleContainerInput")m.name="handleInventoryMouseClick";
    if(owner=="net.minecraft.network.protocol.game.ClientboundSetEntityMotionPacket"){
        if(name=="id")m.name="getId";
        if(name=="movement")m.name="getMovement";
    }
    if(auto it=memberMap.find(Key({owner.c_str(),m.name,desc.c_str()}));it!=memberMap.end())m.name=it->second;
    m.owner=ClassName(m.owner);m.descriptor=Descriptor(desc.c_str());return m;
}
}
