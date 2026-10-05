#include "minecraft/proxy/ProxyHooks.h"
#include "proxy/ProxyService.h"
#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace mock {
template<class T>T Id(int value){return reinterpret_cast<T>(static_cast<uintptr_t>(value));}
bool enabled=true,exception=false,cancel=false,prepareFails=false,screenMissing=false,fakeAddress=false;
int nativeConnects=0,prepares=0,abandons=0,refs=0,lastPort=0;
jobject lastAddress=nullptr;
JNIEnv* current=nullptr;jvmtiEnv* vmti=nullptr;
jclass Class(JNIEnv*,jobject object){return Id<jclass>(object==Id<jobject>(1)?1:object==Id<jobject>(2)?2:object==Id<jobject>(3)?3:4);}
jclass Find(JNIEnv*,const char*){return Id<jclass>(5);}
jint Throw(JNIEnv*,jclass,const char*){exception=true;return 0;}
jboolean Exception(JNIEnv*){return exception?JNI_TRUE:JNI_FALSE;}
void Clear(JNIEnv*){exception=false;}
jfieldID Field(JNIEnv*,jclass,const char* name,const char* desc){assert(std::string(desc)=="Z");assert(std::string(name)=="cancel");return Id<jfieldID>(2);}
jobject ObjectField(JNIEnv*,jobject,jfieldID field){if(field==Id<jfieldID>(3))return Id<jobject>(10);return screenMissing?nullptr:Id<jobject>(2);}
jboolean BoolField(JNIEnv*,jobject screen,jfieldID){assert(screen==Id<jobject>(2));return cancel?JNI_TRUE:JNI_FALSE;}
jobject Global(JNIEnv*,jobject value){++refs;return value;}
void DeleteGlobal(JNIEnv*,jobject){--refs;}
void DeleteLocal(JNIEnv*,jobject){}
jmethodID Method(JNIEnv*,jclass,const char* name,const char*){return Id<jmethodID>(std::string(name)=="connect"?1:2);}
jmethodID StaticMethod(JNIEnv*,jclass,const char*,const char*){return Id<jmethodID>(3);}
jobject ObjectMethod(JNIEnv*,jobject,jmethodID method,va_list args){
    if(method==Id<jmethodID>(1)){++nativeConnects;lastAddress=va_arg(args,jobject);lastPort=va_arg(args,int);return Id<jobject>(8);}
    return Id<jobject>(6);
}
jobject StaticObject(JNIEnv*,jclass,jmethodID,va_list){return Id<jobject>(7);}
const char* Chars(JNIEnv*,jstring text,jboolean*){return text==Id<jstring>(10)?"mc.loyisa.cn":fakeAddress?"198.18.1.6":"198.51.100.25";}
void ReleaseChars(JNIEnv*,jstring,const char*){}
jbyteArray Array(JNIEnv*,jsize count){assert(count==4);return Id<jbyteArray>(9);}
void ArrayRegion(JNIEnv*,jbyteArray,jsize start,jsize count,const jbyte* bytes){assert(start==0&&count==4&&bytes[0]==127&&bytes[3]==1);}
jvmtiError JNICALL Fields(jvmtiEnv*,jclass,jint* count,jfieldID** fields){*count=2;*fields=static_cast<jfieldID*>(std::malloc(2*sizeof(jfieldID)));(*fields)[0]=Id<jfieldID>(1);(*fields)[1]=Id<jfieldID>(3);return JVMTI_ERROR_NONE;}
jvmtiError JNICALL FieldName(jvmtiEnv*,jclass,jfieldID field,char**,char** descriptor,char**){*descriptor=_strdup(field==Id<jfieldID>(3)?"Ljava/lang/String;":"Lnet/minecraft/client/multiplayer/GuiConnecting;");return JVMTI_ERROR_NONE;}
jvmtiError JNICALL Deallocate(jvmtiEnv*,unsigned char* bytes){std::free(bytes);return JVMTI_ERROR_NONE;}
}
namespace nova::compat {
bool IsLunar189(){return true;}bool IsLunar1710(){return false;}bool IsLunar12111(){return false;}
bool IsEaseCation(){return false;}bool IsNeoForge12110(){return false;}bool UsesModernRuntime(){return false;}bool IsLegacyLunar(){return true;}
bool IsMinecraft263(){return false;}
}
bool IsCircuitOpen(){return false;}
jvmtiEnv* CurrentJvmti(){return mock::vmti;}
namespace nova::proxy {
Config ReadConfig(){Config value;value.enabled=mock::enabled;return value;}
void SetSupported(bool){}
void ReportError(const std::string&){}
Prepared Prepare(const Config&,const std::string& target,int port,const Cancel& cancelled){
    ++mock::prepares;assert(target==(mock::fakeAddress?"mc.loyisa.cn":"198.51.100.25")&&port==25565);
    if(mock::prepareFails||cancelled())throw std::runtime_error("proxy unavailable");return {{},34000};
}
void Abandon(const Prepared&){++mock::abandons;}
}
int main(){
    using namespace nova::minecraft::proxyadapter;
    JNINativeInterface_ jni{};jni.GetObjectClass=mock::Class;jni.FindClass=mock::Find;jni.ThrowNew=mock::Throw;
    jni.ExceptionCheck=mock::Exception;jni.ExceptionClear=mock::Clear;jni.GetFieldID=mock::Field;
    jni.GetObjectField=mock::ObjectField;jni.GetBooleanField=mock::BoolField;
    jni.NewGlobalRef=mock::Global;jni.DeleteGlobalRef=mock::DeleteGlobal;jni.DeleteLocalRef=mock::DeleteLocal;
    jni.GetMethodID=mock::Method;jni.GetStaticMethodID=mock::StaticMethod;jni.CallObjectMethodV=mock::ObjectMethod;
    jni.CallStaticObjectMethodV=mock::StaticObject;jni.GetStringUTFChars=mock::Chars;jni.ReleaseStringUTFChars=mock::ReleaseChars;
    jni.NewByteArray=mock::Array;jni.SetByteArrayRegion=mock::ArrayRegion;JNIEnv env{&jni};
    jvmtiInterface_1 ti{};ti.GetClassFields=mock::Fields;ti.GetFieldName=mock::FieldName;ti.Deallocate=mock::Deallocate;
    jvmtiEnv vmti{&ti};mock::vmti=&vmti;Initialize();
    const nova::bridge::HookDef dummy{};int count=0;const auto* hooks=ExtendHooks(&dummy,count);assert(count==3);
    using BeginFn=void(JNICALL*)(JNIEnv*,jclass,jobject);using EndFn=void(JNICALL*)(JNIEnv*,jclass);
    using ConnectFn=jobject(JNICALL*)(JNIEnv*,jclass,jobject,jobject,jint);
    auto begin=reinterpret_cast<BeginFn>(hooks[0].impl);auto end=reinterpret_cast<EndFn>(hooks[1].impl);
    auto connect=reinterpret_cast<ConnectFn>(hooks[2].impl);
    const auto thread=mock::Id<jobject>(1),address=mock::Id<jobject>(3),bootstrap=mock::Id<jobject>(4);
    // An unscoped status ping retains the original endpoint even when enabled.
    connect(&env,nullptr,bootstrap,address,25565);assert(mock::nativeConnects==1&&mock::lastAddress==address);
    begin(&env,nullptr,thread);assert(mock::refs==1);
    connect(&env,nullptr,bootstrap,address,25565);
    assert(mock::nativeConnects==2&&mock::prepares==1&&mock::lastAddress==mock::Id<jobject>(7)&&mock::lastPort==34000);
    end(&env,nullptr);assert(mock::refs==0);
    mock::fakeAddress=true;begin(&env,nullptr,thread);
    connect(&env,nullptr,bootstrap,address,25565);assert(!mock::exception&&mock::lastAddress==mock::Id<jobject>(7));
    end(&env,nullptr);mock::fakeAddress=false;
    for(int failure=0;failure<2;++failure){mock::cancel=failure==1;mock::prepareFails=failure==0;
        begin(&env,nullptr,thread);const int before=mock::nativeConnects;
        assert(connect(&env,nullptr,bootstrap,address,25565)==nullptr&&mock::exception);
        assert(mock::nativeConnects==before);mock::exception=false;end(&env,nullptr);assert(mock::refs==0);
    }
    mock::cancel=mock::prepareFails=false;mock::screenMissing=true;begin(&env,nullptr,thread);assert(mock::exception&&mock::refs==0);
    mock::exception=false;mock::screenMissing=false;mock::enabled=false;begin(&env,nullptr,thread);
    const int before=mock::nativeConnects;connect(&env,nullptr,bootstrap,address,25565);assert(mock::nativeConnects==before+1&&mock::lastAddress==address);
    end(&env,nullptr);mock::enabled=true;begin(&env,nullptr,thread);assert(mock::refs==1);
    Shutdown(&env);assert(mock::refs==0);
    std::cout<<"PASS: actual JNI proxy adapter preserves unscoped/disabled destinations, scopes joins, substitutes loopback, fails closed on authentication/cancel and releases exceptional scopes\n";
}
