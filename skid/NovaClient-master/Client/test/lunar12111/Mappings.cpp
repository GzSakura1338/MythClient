#include "minecraft/compat/lunar12111/Mapping.h"
#include <iostream>
namespace catalog {
#define NOVA_CLASS(id,name) constexpr const char* id=name;
#define NOVA_FIELD(...)
#define NOVA_STATIC_FIELD(...)
#define NOVA_METHOD(...)
#define NOVA_STATIC_METHOD(...)
#include "minecraft/compat/mappings/minecraft263.inc"
#undef NOVA_CLASS
#undef NOVA_FIELD
#undef NOVA_STATIC_FIELD
#undef NOVA_METHOD
#undef NOVA_STATIC_METHOD
}
int main(){
    using namespace catalog;
    auto print=[](const char* id,const char* owner,const char* name,const char* desc){
        auto m=nova::compat::lunar12111::Translate({owner,name,desc});
        std::cout<<id<<'\t'<<owner<<'\t'<<name<<'\t'<<desc<<'\t'<<m.owner<<'\t'<<m.name<<'\t'<<m.descriptor<<'\n';
    };
#define NOVA_CLASS(id,name) print("class:" #id,name,"","");
#define NOVA_FIELD(id,owner,name,desc) print(#id,owner,name,desc);
#define NOVA_STATIC_FIELD NOVA_FIELD
#define NOVA_METHOD NOVA_FIELD
#define NOVA_STATIC_METHOD NOVA_FIELD
#include "minecraft/compat/mappings/minecraft263.inc"
#undef NOVA_CLASS
#undef NOVA_FIELD
#undef NOVA_STATIC_FIELD
#undef NOVA_METHOD
#undef NOVA_STATIC_METHOD
#define NOVA_EASECATION_MEMBER(id,owner,name,desc) print("ec_" #id,owner,name,desc);
#include "minecraft/compat/mappings/easecation.inc"
#undef NOVA_EASECATION_MEMBER
#include "extra.inc"
}
