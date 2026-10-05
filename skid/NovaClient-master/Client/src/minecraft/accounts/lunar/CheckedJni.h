#pragma once
#include "minecraft/compat/Profile.h"
#include "minecraft/accounts/lunar/AccountMapping.h"
#include <algorithm>
#include <string>

namespace nova::minecraft::accounts {
// Never log auth exceptions: their messages may contain tokens or HTTP payloads.
struct JniFailure {};
class Jni {
public:
    JNIEnv* e; jobject loader;
    explicit Jni(JNIEnv* env):e(env),loader(nullptr) {
        if(e->PushLocalFrame(4096)!=JNI_OK) { e->ExceptionClear();throw JniFailure{}; }
        loader=compat::ClassLoader(e);
        if(!loader){e->PopLocalFrame(nullptr);throw JniFailure{};}
    }
    ~Jni(){e->DeleteGlobalRef(loader);e->PopLocalFrame(nullptr);}
    void check(){if(e->ExceptionCheck()){e->ExceptionClear();throw JniFailure{};}}
    template<class T>T required(T value){check();if(!value)throw JniFailure{};return value;}
    static std::string descriptor(std::string name){std::replace(name.begin(),name.end(),'.','/');return "L"+name+";";}
    static std::string returns(const char* name){return "()"+descriptor(name);}
    jclass cls(const char* name){auto mapped=mapping::Remap(name);std::replace(mapped.begin(),mapped.end(),'/','.');return required(LoadClassInLoader(e,loader,mapped.c_str()));}
    jclass type(jobject object){return required(e->GetObjectClass(required(object)));}
    jmethodID method(jobject o,const char* name,const char* sig){return required(e->GetMethodID(type(o),mapping::Remap(name).c_str(),mapping::Remap(sig).c_str()));}
    jfieldID field(jobject o,const char* name,const char* sig){return required(e->GetFieldID(type(o),mapping::Remap(name).c_str(),mapping::Remap(sig).c_str()));}
    jobject get(jobject o,const char* name,const char* sig){auto id=field(o,name,sig);auto r=e->GetObjectField(o,id);check();return r;}
    jobject constant(const char* name,const char* member,const char* sig){auto c=cls(name);auto f=required(e->GetStaticFieldID(c,mapping::Remap(member).c_str(),mapping::Remap(sig).c_str()));auto r=e->GetStaticObjectField(c,f);check();return r;}
    jstring str(const std::string& value){return required(e->NewStringUTF(value.c_str()));}
    std::string text(jobject value){if(!value)return {};auto s=static_cast<jstring>(value);auto p=required(e->GetStringUTFChars(s,nullptr));std::string out(p);e->ReleaseStringUTFChars(s,p);return out;}
    template<class... A>jobject call(jobject o,const char* name,const char* sig,A... a){auto m=method(o,name,sig);auto r=e->CallObjectMethod(o,m,a...);check();return r;}
    template<class... A>void run(jobject o,const char* name,const char* sig,A... a){auto m=method(o,name,sig);e->CallVoidMethod(o,m,a...);check();}
    bool flag(jobject o,const char* name){auto m=method(o,name,"()Z");auto r=e->CallBooleanMethod(o,m);check();return r!=0;}
    template<class... A>jobject stat(const char* name,const char* member,const char* sig,A... a){auto c=cls(name);auto m=required(e->GetStaticMethodID(c,mapping::Remap(member).c_str(),mapping::Remap(sig).c_str()));auto r=e->CallStaticObjectMethod(c,m,a...);check();return r;}
    void staticRun(const char* name,const char* member){auto c=cls(name);auto m=required(e->GetStaticMethodID(c,mapping::Remap(member).c_str(),"()V"));e->CallStaticVoidMethod(c,m);check();}
    template<class... A>jobject make(const char* name,const char* sig,A... a){auto c=cls(name);auto m=required(e->GetMethodID(c,"<init>",mapping::Remap(sig).c_str()));return required(e->NewObject(c,m,a...));}
    jobject global(jobject o){return o?required(e->NewGlobalRef(o)):nullptr;}
};
}
