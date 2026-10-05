#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <jni.h>
#include "minecraft/java/api/class_edit.h"
#include <fstream>
#include <iostream>

static bool cancelTick;
static jboolean JNICALL Cancel(JNIEnv*, jclass, jobject) { return cancelTick ? JNI_TRUE : JNI_FALSE; }

// Uses the existing compiled bytecode fixture; creates no Java source files.
int main(int argc, char** argv) {
    if(argc!=3)return 2;
    auto library=LoadLibraryA(argv[1]);
    if(!library)return 3;
    auto create=reinterpret_cast<jint(JNICALL*)(JavaVM**,void**,void*)>(GetProcAddress(library,"JNI_CreateJavaVM"));
    JavaVMOption option{const_cast<char*>("-Xverify:all"),nullptr};
    JavaVMInitArgs args{};args.version=JNI_VERSION_1_8;args.nOptions=1;args.options=&option;
    JavaVM* vm=nullptr;JNIEnv* env=nullptr;
    if(!create || create(&vm,reinterpret_cast<void**>(&env),&args)!=JNI_OK)return 4;
    ClassBuilder bridge("EntryGuardBridge","java/lang/Object",52);
    bridge.addNativeMethod("cancel","(Ljava/lang/Object;)Z");
    auto bytes=bridge.build();
    auto hook=env->DefineClass("EntryGuardBridge",nullptr,reinterpret_cast<const jbyte*>(bytes.data()),bytes.size());
    JNINativeMethod native{const_cast<char*>("cancel"),const_cast<char*>("(Ljava/lang/Object;)Z"),reinterpret_cast<void*>(&Cancel)};
    if(!hook || env->RegisterNatives(hook,&native,1)!=JNI_OK)return 5;
    std::ifstream input(argv[2],std::ios::binary);
    bytes.assign(std::istreambuf_iterator<char>(input),{});
    ClassFileEditor editor(bytes.data(),bytes.size());
    for(const char* name:{"guardedTickBusy","tailVoid"})
        if(!editor.insertCancelGuard(name,"()V","EntryGuardBridge","cancel","(Ljava/lang/Object;)Z",1,true))return 6;
    bytes=editor.serialize();
    auto probe=env->DefineClass("Probe",nullptr,reinterpret_cast<const jbyte*>(bytes.data()),bytes.size());
    if(!probe){env->ExceptionDescribe();return 7;}
    auto object=env->AllocObject(probe);
    auto touched=env->GetFieldID(probe,"touched","I");
    auto tick=env->GetMethodID(probe,"guardedTickBusy","()V");
    auto straight=env->GetMethodID(probe,"tailVoid","()V");
    if(!object || !touched || !tick || !straight){env->ExceptionDescribe();return 8;}
    cancelTick=true;env->CallVoidMethod(object,tick);env->CallVoidMethod(object,straight);
    if(env->ExceptionCheck() || env->GetIntField(object,touched)!=0){env->ExceptionDescribe();return 9;}
    cancelTick=false;env->CallVoidMethod(object,tick);env->CallVoidMethod(object,straight);
    if(env->ExceptionCheck() || env->GetIntField(object,touched)!=18){env->ExceptionDescribe();return 10;}
    vm->DestroyJavaVM();
    std::cout<<"PASS: JVM verification, cancel skips body, continue preserves body, live tail locals and no-frame method\n";
}
