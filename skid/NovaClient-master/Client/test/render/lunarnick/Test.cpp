#include "minecraft/java/api/game_presentation_name.h"
#include "minecraft/compat/mappings/LunarNick.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include <cassert>
#include <cstdarg>
#include <iostream>
#include <string>
namespace mock {
bool lunar=true,legacy=false,old17=false,enabled=true,exception=false,available=true,fail=false;
bool october=true;
int calls=0,frames=0,strings=0;
std::u16string input,result;
std::string nick="You";
JNIEnv* env=nullptr;
jvmtiEnv* vmti=nullptr;
template<class T>T Id(int n){return reinterpret_cast<T>(static_cast<uintptr_t>(n));}
jboolean Exception(JNIEnv*){return exception?JNI_TRUE:JNI_FALSE;}
void Clear(JNIEnv*){exception=false;}
jint Push(JNIEnv*,jint){++frames;return JNI_OK;}
jobject Pop(JNIEnv*,jobject v){--frames;return v;}
jobject Static(JNIEnv*,jclass,jmethodID id,va_list){assert(id==Id<jmethodID>(1));return available?Id<jobject>(1):nullptr;}
jobject Call(JNIEnv*,jobject obj,jmethodID id,va_list){
    if(id==Id<jmethodID>(2)){assert(obj==Id<jobject>(1));return Id<jobject>(2);}
    if(id==Id<jmethodID>(3)){assert(obj==Id<jobject>(2));return Id<jobject>(3);}
    assert(id==Id<jmethodID>(4)&&obj==Id<jobject>(3));++calls;
    if(fail){exception=true;return nullptr;}
    result=input;if(enabled){const std::u16string name=u"RealUser";const auto at=result.find(name);
        if(at!=result.npos)result.replace(at,name.size(),std::u16string(nick.begin(),nick.end()));}
    return Id<jobject>(5);
}
jstring New(JNIEnv*,const jchar* value,jsize length){input.assign(reinterpret_cast<const char16_t*>(value),length);++strings;return Id<jstring>(4);}
jsize Length(JNIEnv*,jstring){return jsize(result.size());}
const jchar* Chars(JNIEnv*,jstring,jboolean*){return reinterpret_cast<const jchar*>(result.data());}
void Release(JNIEnv*,jstring,const jchar*){--strings;}
jobject GetField(JNIEnv*,jobject obj,jfieldID){assert(obj==Id<jobject>(1));return available?Id<jobject>(3):nullptr;}
jclass GetClass(JNIEnv*,jobject){return Id<jclass>(10);}
jclass Parent(JNIEnv*,jclass){return nullptr;}
void Delete(JNIEnv*,jobject){}
jvmtiError JNICALL Methods(jvmtiEnv*,jclass,jint* count,jmethodID** methods){*count=1;*methods=static_cast<jmethodID*>(std::malloc(sizeof(jmethodID)));**methods=Id<jmethodID>(4);return JVMTI_ERROR_NONE;}
jvmtiError JNICALL Name(jvmtiEnv*,jmethodID,char** name,char** signature,char**){*name=_strdup("localvar$changedbuild$lunar$renderString");*signature=_strdup("(Ljava/lang/String;)Ljava/lang/String;");return JVMTI_ERROR_NONE;}
jvmtiError JNICALL Deallocate(jvmtiEnv*,unsigned char* memory){std::free(memory);return JVMTI_ERROR_NONE;}
}
JniAttach::JniAttach(){env=mock::env;}JniAttach::~JniAttach()=default;
jvmtiEnv* CurrentJvmti(){return mock::vmti;}
namespace nova::compat {
bool IsLegacyLunar(){return mock::lunar&&mock::legacy;}bool IsLunar12111(){return mock::lunar&&!mock::legacy;}
bool IsLunar1710(){return mock::old17;}jobject Minecraft(JNIEnv*){return mock::Id<jobject>(1);}
}
namespace nova::compat::mc263::access {
jclass Class(JNIEnv*,const char* name){assert(std::string(name)==lunarnick::Utility);return mock::Id<jclass>(10);}
jmethodID Method(JNIEnv*,Member m,bool stat){
    using namespace lunarnick;
    for(int b=0;b<2;++b){const auto& build=ModernBuilds[b];const Member expected[]{build.instance,build.modules,build.nickHider,build.format};
        for(int i=0;i<4;++i)if(std::string(m.owner)==expected[i].owner&&std::string(m.name)==expected[i].name&&std::string(m.descriptor)==expected[i].descriptor){
            assert(stat==(i==0));if((b==0)!=mock::october){mock::exception=true;return nullptr;}return mock::Id<jmethodID>(i+1);
        }
    }
    assert(false);return nullptr;
}
jfieldID Field(JNIEnv*,Member m,bool){
    assert(std::string(m.owner)=="net.minecraft.client.Minecraft");
    assert(std::string(m.name)=="fontRendererObj");
    assert(std::string(m.descriptor)=="Lnet/minecraft/client/gui/FontRenderer;");return mock::Id<jfieldID>(5);
}
}
int main(){
    JNINativeInterface_ api{};api.ExceptionCheck=mock::Exception;api.ExceptionClear=mock::Clear;
    api.PushLocalFrame=mock::Push;api.PopLocalFrame=mock::Pop;api.CallStaticObjectMethodV=mock::Static;
    api.CallObjectMethodV=mock::Call;api.NewString=mock::New;api.GetStringLength=mock::Length;
    api.GetStringChars=mock::Chars;api.ReleaseStringChars=mock::Release;
    api.GetObjectField=mock::GetField;api.GetObjectClass=mock::GetClass;api.GetSuperclass=mock::Parent;api.DeleteLocalRef=mock::Delete;
    jvmtiInterface_1 ti{};ti.GetClassMethods=mock::Methods;ti.GetMethodName=mock::Name;ti.Deallocate=mock::Deallocate;
    jvmtiEnv vmti{&ti};mock::vmti=&vmti;
    JNIEnv env{&api};mock::env=&env;using nova::java::PresentationName;
    assert(PresentationName("RealUser")=="You");
    mock::october=false;assert(PresentationName("RealUser")=="You"&&!mock::exception);mock::october=true;
    mock::nick="HiddenPlayer";assert(PresentationName("RealUser")=="HiddenPlayer");
    assert(PresentationName("§a[MVP+] RealUser")=="§a[MVP+] HiddenPlayer");
    assert(PresentationName("中文 RealUser 🐱")=="中文 HiddenPlayer 🐱");
    mock::enabled=false;assert(PresentationName("RealUser")=="RealUser");mock::enabled=true;
    mock::available=false;assert(PresentationName("RealUser")=="RealUser");mock::available=true;
    mock::fail=true;assert(PresentationName("RealUser")=="RealUser");assert(!mock::exception);mock::fail=false;mock::strings=0;
    mock::exception=true;const int before=mock::calls;assert(PresentationName("RealUser")=="RealUser");assert(mock::exception&&mock::calls==before);mock::exception=false;
    for(bool old17:{false,true}){mock::legacy=true;mock::old17=old17;mock::nick="You";
        assert(PresentationName("RealUser")=="You");mock::enabled=false;assert(PresentationName("RealUser")=="RealUser");mock::enabled=true;
    }
    const int afterLegacy=mock::calls;
    mock::lunar=false;assert(PresentationName("RealUser")=="RealUser"&&mock::calls==afterLegacy);
    assert(mock::frames==0);std::cout<<"PASS: live Lunar nick formatter, toggles, formatting/Unicode, non-Lunar isolation, unavailable adapter and exception ownership\n";
}
