#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/compat/lunar12111/Mapping.h"
#include "minecraft/compat/easecation/EaseCation.h"
#include <cassert>
#include <cstdio>

namespace nova::compat {
bool IsLunar12111(){return false;}
bool IsEaseCation(){return false;}
bool IsNeoForge12110(){return false;}
bool IsLunar1710(){return false;}
jobject ClassLoader(JNIEnv*){return nullptr;}
namespace lunar12111 { Member Translate(Member m){return m;} }
namespace easecation {
const char* ClassName(const char* name){return name;}
mc263::names::Member Translate(mc263::names::Member m){return m;}
}
namespace neoforge12110 {
const char* ClassName(const char* name){return name;}
mc263::names::Member Translate(mc263::names::Member m){return m;}
}
}
void LogTo(const char*,...){}
int classLoads=0,fieldLookups=0,methodLookups=0;
jclass LoadClassInLoader(JNIEnv*,jobject,const char*){++classLoads;return reinterpret_cast<jclass>(1);}
jboolean JNICALL ExceptionCheck(JNIEnv*){return JNI_FALSE;}
jobject JNICALL NewGlobalRef(JNIEnv*,jobject value){return value;}
void JNICALL DeleteRef(JNIEnv*,jobject){}
jfieldID JNICALL GetField(JNIEnv*,jclass,const char*,const char*){++fieldLookups;return reinterpret_cast<jfieldID>(2);}
jmethodID JNICALL GetMethod(JNIEnv*,jclass,const char*,const char*){++methodLookups;return reinterpret_cast<jmethodID>(3);}

int main(){
    using namespace nova::compat::mc263::access;
    JNINativeInterface_ table{};
    table.ExceptionCheck=ExceptionCheck;table.NewGlobalRef=NewGlobalRef;
    table.DeleteLocalRef=DeleteRef;table.DeleteGlobalRef=DeleteRef;
    table.GetFieldID=table.GetStaticFieldID=GetField;
    table.GetMethodID=table.GetStaticMethodID=GetMethod;
    JNIEnv env{&table};
    // Reproduce an omitted jump entry and each partially populated mapping.
    const Member invalid[]{ {}, {nullptr,"jump","I"},{"Options",nullptr,"I"},
        {"Options","jump",nullptr},{"","jump","I"},{"Options","","I"},{"Options","jump",""} };
    for(auto m:invalid){
        assert(!Field(&env,m));assert(!Field(&env,m,true));
        assert(!Method(&env,m));assert(!Method(&env,m,true));
    }
    assert(!Class(&env,nullptr)&&!Class(&env,""));
    const Member field{"Options","jump","I"},method{"Options","getJump","()I"};
    assert(!Class(nullptr,"Options")&&!Field(nullptr,field)&&!Method(nullptr,method));
    assert(classLoads==0&&fieldLookups==0&&methodLookups==0);
    assert(Field(&env,field)&&Field(&env,field));
    assert(Method(&env,method)&&Method(&env,method));
    assert(Field(&env,field,true)&&Method(&env,method,true));
    assert(classLoads==1&&fieldLookups==2&&methodLookups==2);
    Release(&env);
    std::puts("PASS: actual JNI access rejects empty/partial mappings before JNI/cache access; valid lookups and static caches preserved");
}
