// Execute the real cached enum in an isolated JVM. Symbol existence alone
// cannot detect 1.7's misleading EAST/WEST names.
#define NOMINMAX
#include <Windows.h>
#include "jni.h"
#include "audit.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string_view>

struct Binding { const char* id; const char* owner; const char* name; const char* desc; };
const Binding fields[]{
#define NOVA_LUNAR1710_RUNTIME
#define NOVA_CLASS(...)
#define NOVA_FIELD(...)
#define NOVA_METHOD(...)
#define NOVA_STATIC_METHOD(...)
#define NOVA_STATIC_FIELD(id,owner,name,desc) {#id,#owner,name,desc},
#include "minecraft/compat/mappings/Lunar1710.inc"
#undef NOVA_STATIC_FIELD
#undef NOVA_STATIC_METHOD
#undef NOVA_METHOD
#undef NOVA_FIELD
#undef NOVA_CLASS
#undef NOVA_LUNAR1710_RUNTIME
};

int main(int argc,char** argv) {
    if(argc!=2) return 2;
    std::ifstream input("classes/net.minecraft.util.EnumFacing",std::ios::binary);
    std::vector<u1> bytes{std::istreambuf_iterator<char>(input),{}};
    if(bytes.size()<10) return 3;
    const u1* cursor=bytes.data()+8;
    audit::Pool cp;
    if(!audit::ParsePool(cursor,bytes.data()+bytes.size(),cp)) return 4;
    const size_t interfaces=cursor-bytes.data()+6;
    const auto count=audit::Be16(bytes.data()+interfaces);
    // Only detach Lunar's bridge interface; preserve all real fields, methods,
    // constructors and enum initialization instructions unchanged.
    bytes.erase(bytes.begin()+interfaces+2,bytes.begin()+interfaces+2+count*2);
    bytes[interfaces]=bytes[interfaces+1]=0;
    const auto jvmPath=std::filesystem::path(argv[1]);
    SetDllDirectoryW(jvmPath.parent_path().parent_path().c_str());
    auto library=LoadLibraryW(jvmPath.c_str());
    using Create=jint(JNICALL*)(JavaVM**,void**,void*);
    auto create=library?reinterpret_cast<Create>(GetProcAddress(library,"JNI_CreateJavaVM")):nullptr;
    JavaVMOption options[]={{const_cast<char*>("-Xverify:all"),nullptr},{const_cast<char*>("-Xmx64m"),nullptr}};
    JavaVMInitArgs args{JNI_VERSION_1_8,2,options,JNI_FALSE};
    JavaVM* vm=nullptr; JNIEnv* env=nullptr;
    if(!create || create(&vm,reinterpret_cast<void**>(&env),&args)!=JNI_OK) return 5;
    auto cls=env->DefineClass("net/minecraft/util/EnumFacing",nullptr,reinterpret_cast<const jbyte*>(bytes.data()),static_cast<jsize>(bytes.size()));
    if(!cls || env->ExceptionCheck()) {env->ExceptionDescribe();return 6;}
    const auto ordinal=env->GetMethodID(cls,"ordinal","()I");
    const std::array<jmethodID,3> step{env->GetMethodID(cls,"getFrontOffsetX","()I"),env->GetMethodID(cls,"getFrontOffsetY","()I"),env->GetMethodID(cls,"getFrontOffsetZ","()I")};
    constexpr const char* ids[]{"FacingDown","FacingUp","FacingNorth","FacingSouth","FacingWest","FacingEast"};
    constexpr int expected[][3]={{0,-1,0},{0,1,0},{0,0,-1},{0,0,1},{-1,0,0},{1,0,0}};
    int failures=0;
    for(int side=0;side<6;++side) {
        const Binding* binding=nullptr;
        for(const auto& b:fields) if(std::string_view(b.id)==ids[side]) binding=&b;
        if(!binding) {++failures;continue;}
        auto field=env->GetStaticFieldID(cls,binding->name,binding->desc);
        auto facing=field?env->GetStaticObjectField(cls,field):nullptr;
        if(!facing || env->ExceptionCheck()) {env->ExceptionDescribe();return 7;}
        bool valid=env->CallIntMethod(facing,ordinal)==side;
        for(int axis=0;axis<3;++axis) valid &= env->CallIntMethod(facing,step[axis])==expected[side][axis];
        if(!valid) {++failures;std::cerr<<"FAIL "<<ids[side]<<" mapped to "<<binding->name<<": side/offset mismatch\n";}
        env->DeleteLocalRef(facing);
    }
    vm->DestroyJavaVM();
    if(!failures) std::cout<<"PASS: all six 1.7 face IDs and XYZ offsets match actual enum execution\n";
    return failures?1:0;
}
