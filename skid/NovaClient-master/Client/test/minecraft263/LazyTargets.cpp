// Native-only JVM integration test: execute the real Transformer with fourteen
// loaded targets and one lazy target, matching a 26.3 main-menu installation.
#include "minecraft/java/api/base.h"
#include "minecraft/java/api/classfile.h"
#include "minecraft/hooks/Edits.h"
#include "minecraft/hooks/Transformer.h"
#include <array>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace {
std::array<std::string,15> names;
std::array<nova::hook::ClassEdits,15> edits;
const nova::hook::Append append{"tick","()V","called","()V",nullptr,0,true};
jobject loader = nullptr;
int initialized = 0, calls = 0;
void JNICALL Initialized(JNIEnv*,jclass) { ++initialized; }
void JNICALL Called(JNIEnv*,jclass) { ++calls; }
void WriteClass(const std::string& name, const std::vector<u1>& bytes) {
    auto path = std::filesystem::path("fixtures")/(name+".class");
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path,std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
}
}
void LogTo(const char* format,...) {
    va_list args; va_start(args,format); std::vprintf(format,args); va_end(args); std::puts("");
}
namespace nova::compat {
bool IsLegacyLunar() { return false; }
bool IsMinecraft263() { return true; }
bool UsesModernRuntime() { return true; }
jobject ClassLoader(JNIEnv* env) { return env->NewGlobalRef(loader); }
}
namespace nova::bridge {
const std::string& InternalName() { static const std::string name="test/Bridge"; return name; }
}
jclass LoadClassInLoader(JNIEnv* env,jobject selected,const char* name) {
    auto type=env->GetObjectClass(selected);
    auto method=env->GetMethodID(type,"loadClass","(Ljava/lang/String;)Ljava/lang/Class;");
    auto text=env->NewStringUTF(name);
    auto result=static_cast<jclass>(env->CallObjectMethod(selected,method,text));
    env->DeleteLocalRef(text);env->DeleteLocalRef(type);
    return result;
}
namespace nova::hook {
const ClassEdits* AllEdits(int& count) { count=static_cast<int>(edits.size()); return edits.data(); }
const ClassEdits* EditsFor(const char* name) {
    for (auto& e:edits) if (std::strcmp(name,e.internalName)==0) return &e;
    return nullptr;
}
std::vector<unsigned char> AppendCode(std::uint16_t ref,const StackLoad*,int count) {
    assert(count==0); return {0xB8,static_cast<u1>(ref>>8),static_cast<u1>(ref)};
}
}
int main(int argc,char** argv) {
    if(argc!=3) return 2;
    const bool missing=std::string_view(argv[2])=="missing";
    const bool broken=std::string_view(argv[2])=="broken";
    ClassBuilder bridge("test/Bridge","java/lang/Object");
    bridge.addNativeMethod("initialized","()V");bridge.addNativeMethod("called","()V");
    WriteClass("test/Bridge",bridge.build());
    for(int i=0;i<15;++i) {
        names[i]="test/Target"+std::to_string(i);
        edits[i]={names[i].c_str(),nullptr,0,nullptr,0,nullptr,0,&append,1};
        if(missing && i==14) continue;
        ClassBuilder target(names[i],"java/lang/Object");
        const auto init=target.methodRef("test/Bridge","initialized","()V");
        target.addCodedMethod("<clinit>","()V",ACC_STATIC,{0xB8,static_cast<u1>(init>>8),static_cast<u1>(init),0xB1},0,0);
        target.addCodedMethod(broken&&i==14?"changed":"tick","()V",ACC_PUBLIC|ACC_STATIC,{0xB1},0,0);
        WriteClass(names[i],target.build());
    }
    auto path=std::filesystem::path(argv[1]);
    SetDllDirectoryW(path.parent_path().parent_path().c_str());
    auto library=LoadLibraryW(path.c_str());
    using Create=jint(JNICALL*)(JavaVM**,void**,void*);
    auto create=library?reinterpret_cast<Create>(GetProcAddress(library,"JNI_CreateJavaVM")):nullptr;
    JavaVMOption options[]={{const_cast<char*>("-Xverify:all"),nullptr},
        {const_cast<char*>("-Djava.class.path=fixtures"),nullptr}};
    JavaVMInitArgs args{JNI_VERSION_1_8,2,options,JNI_FALSE};
    JavaVM* vm=nullptr;JNIEnv* env=nullptr;
    assert(create && create(&vm,reinterpret_cast<void**>(&env),&args)==JNI_OK);
    jvmtiEnv* jvmti=nullptr;
    assert(vm->GetEnv(reinterpret_cast<void**>(&jvmti),JVMTI_VERSION_1_2)==JNI_OK);
    auto bridgeClass=env->FindClass("test/Bridge");
    JNINativeMethod bindings[]={{const_cast<char*>("initialized"),const_cast<char*>("()V"),reinterpret_cast<void*>(Initialized)},
        {const_cast<char*>("called"),const_cast<char*>("()V"),reinterpret_cast<void*>(Called)}};
    assert(env->RegisterNatives(bridgeClass,bindings,2)==JNI_OK);
    auto cl=env->FindClass("java/lang/ClassLoader");
    loader=env->CallStaticObjectMethod(cl,env->GetStaticMethodID(cl,"getSystemClassLoader","()Ljava/lang/ClassLoader;"));
    for(int i=0;i<14;++i) {
        auto cls=env->FindClass(names[i].c_str()); assert(cls);
        env->DeleteLocalRef(cls);
    }
    jint count=0; jclass* classes=nullptr;
    assert(jvmti->GetLoadedClasses(&count,&classes)==JVMTI_ERROR_NONE);
    int loaded=0;
    for(int i=0;i<count;++i) {
        char* sig=nullptr;
        if(jvmti->GetClassSignature(classes[i],&sig,nullptr)==JVMTI_ERROR_NONE && sig) {
            if(std::string_view(sig).starts_with("Ltest/Target")) ++loaded;
            jvmti->Deallocate(reinterpret_cast<unsigned char*>(sig));
        }
        env->DeleteLocalRef(classes[i]);
    }
    jvmti->Deallocate(reinterpret_cast<unsigned char*>(classes));
    assert(loaded==14 && initialized==14);
    const bool installed=nova::hook::EnableTransformer(jvmti,env);
    assert(!env->ExceptionCheck());
    if(missing || broken) {
        assert(!installed);
        if(missing) assert(nova::hook::TransformedCount()==0);
        assert(nova::hook::DisableTransformer(jvmti,env));
        std::puts("PASS: unavailable targets / unmatched rules still reject installation");
    } else {
        assert(installed && nova::hook::TransformedCount()==15);
        assert(initialized==14); // Preload must not run target static initializers.
        auto late=env->FindClass(names[14].c_str());
        auto tick=env->GetStaticMethodID(late,"tick","()V");
        env->CallStaticVoidMethod(late,tick);
        assert(!env->ExceptionCheck() && initialized==15 && calls==1);
        assert(nova::hook::DisableTransformer(jvmti,env));
        env->CallStaticVoidMethod(late,tick);
        assert(!env->ExceptionCheck() && calls==1);
        std::puts("PASS: 14 loaded + 1 lazy target, all 15 hooks installed, no eager initialization, hook executes, unload restores");
    }
    vm->DestroyJavaVM();return 0;
}
