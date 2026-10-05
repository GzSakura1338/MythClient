#include "minecraft/modules/misc/imefix/ChatInput.h"
#include "minecraft/compat/lunar12111/Mapping.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <string>

namespace mock {
bool legacy=true,exception=false,clientThread=true,focused=true,enabled=true,chat=true;
bool failInsert=false,missing=false;
int globals=0,insertions=0;
jobject screen=reinterpret_cast<jobject>(10),world=reinterpret_cast<jobject>(20),input=reinterpret_cast<jobject>(30);
std::u16string committed;
template<class T>T Id(int value){return reinterpret_cast<T>(static_cast<uintptr_t>(value));}
jmethodID Method(JNIEnv*,jclass type,const char* name,const char* desc){
    const std::string n=name,d=desc;
    auto expected=[&](const char* owner,const char* oldName,const char* newName,const char* sig,int id){
        const auto member=legacy?nova::compat::lunar12111::Member{owner,oldName,sig}:
            nova::compat::lunar12111::Translate({owner,newName,sig});
        if(n==member.name && d==member.descriptor)return Id<jmethodID>(id);
        return jmethodID{};
    };
    jmethodID id=nullptr;
    if(type==Id<jclass>(1)) {
        id=expected("net.minecraft.client.Minecraft","getMinecraft","getInstance","()Lnet/minecraft/client/Minecraft;",1);
        if(!id){
            const auto m=legacy?nova::compat::lunar12111::Member{"net.minecraft.client.Minecraft","isCallingFromMinecraftThread","()Z"}:
                nova::compat::lunar12111::Translate({"net.minecraft.util.thread.BlockableEventLoop","isSameThread","()Z"});
            if(n==m.name && d==m.descriptor)id=Id<jmethodID>(2);
        }
    } else if(type==Id<jclass>(3)) {
        const auto owner=legacy?"net.minecraft.client.gui.GuiTextField":"net.minecraft.client.gui.components.EditBox";
        id=expected(owner,"isFocused","canConsumeInput","()Z",3);
        if(!id)id=expected(owner,"writeText","insertText","(Ljava/lang/String;)V",4);
        if(!id)id=expected(owner,"getText","getValue","()Ljava/lang/String;",5);
    }
    if(!id || (missing && id==Id<jmethodID>(4))){exception=true;return nullptr;}
    return id;
}
jfieldID Field(JNIEnv*,jclass type,const char* name,const char* desc){
    struct Binding{int type;const char* owner;const char* oldName;const char* newName;const char* oldDesc;const char* newDesc;int id;};
    const Binding bindings[]{
        {1,"net.minecraft.client.Minecraft","currentScreen","screen","Lnet/minecraft/client/gui/GuiScreen;","Lnet/minecraft/client/gui/screens/Screen;",1},
        {1,"net.minecraft.client.Minecraft","theWorld","level","Lnet/minecraft/client/multiplayer/WorldClient;","Lnet/minecraft/client/multiplayer/ClientLevel;",2},
        {2,legacy?"net.minecraft.client.gui.GuiChat":"net.minecraft.client.gui.screens.ChatScreen","inputField","input","Lnet/minecraft/client/gui/GuiTextField;","Lnet/minecraft/client/gui/components/EditBox;",3},
        {3,"net.minecraft.client.gui.GuiTextField","isEnabled","","Z","",4}
    };
    for(const auto& b:bindings){
        if(type!=Id<jclass>(b.type)||(!legacy&&b.id==4))continue;
        const auto m=legacy?nova::compat::lunar12111::Member{b.owner,b.oldName,b.oldDesc}:
            nova::compat::lunar12111::Translate({b.owner,b.newName,b.newDesc});
        if(std::string(name)==m.name && std::string(desc)==m.descriptor)return Id<jfieldID>(b.id);
    }
    exception=true;return nullptr;
}
jboolean Exception(JNIEnv*){return exception?JNI_TRUE:JNI_FALSE;}
void ClearException(JNIEnv*){exception=false;}
jobject Global(JNIEnv*,jobject obj){if(obj)++globals;return obj;}
void DeleteGlobal(JNIEnv*,jobject obj){if(obj)--globals;}
void DeleteLocal(JNIEnv*,jobject){}
jobject StaticObject(JNIEnv*,jclass,jmethodID,va_list){return Id<jobject>(100);}
jobject GetField(JNIEnv*,jobject,jfieldID field){
    if(field==Id<jfieldID>(1))return screen;
    if(field==Id<jfieldID>(2))return world;
    if(field==Id<jfieldID>(3))return input;
    assert(false);return nullptr;
}
jboolean BoolMethod(JNIEnv*,jobject,jmethodID id,va_list){return (id==Id<jmethodID>(2)?clientThread:focused)?JNI_TRUE:JNI_FALSE;}
jboolean BoolField(JNIEnv*,jobject,jfieldID){return enabled?JNI_TRUE:JNI_FALSE;}
jboolean Instance(JNIEnv*,jobject,jclass){return chat?JNI_TRUE:JNI_FALSE;}
jboolean Same(JNIEnv*,jobject a,jobject b){return a==b?JNI_TRUE:JNI_FALSE;}
jstring NewString(JNIEnv*,const jchar* text,jsize size){committed.assign(reinterpret_cast<const char16_t*>(text),size);return Id<jstring>(200);}
void VoidMethod(JNIEnv*,jobject obj,jmethodID id,va_list){assert(obj==input && id==Id<jmethodID>(4));++insertions;if(failInsert)exception=true;}
jobject ObjectMethod(JNIEnv*,jobject,jmethodID id,va_list){assert(id==Id<jmethodID>(5));return Id<jobject>(200);}
jsize Length(JNIEnv*,jstring){return static_cast<jsize>(committed.size());}
}
namespace nova::compat {bool IsLunar189(){return mock::legacy;}}
void LogTo(const char*,...){}
jobject GetMinecraftClassLoader(JNIEnv*){++mock::globals;return mock::Id<jobject>(300);}
jclass LoadClassInLoader(JNIEnv*,jobject,const char* name){
    const char* names[]{"net.minecraft.client.Minecraft",mock::legacy?"net.minecraft.client.gui.GuiChat":"net.minecraft.client.gui.screens.ChatScreen",
        mock::legacy?"net.minecraft.client.gui.GuiTextField":"net.minecraft.client.gui.components.EditBox"};
    for(int i=0;i<3;++i)if(std::string(name)==(mock::legacy?names[i]:nova::compat::lunar12111::ClassName(names[i])))return mock::Id<jclass>(i+1);
    return nullptr;
}
int main(){
    JNINativeInterface_ functions{};
    functions.GetMethodID=mock::Method;functions.GetStaticMethodID=mock::Method;functions.GetFieldID=mock::Field;
    functions.ExceptionCheck=mock::Exception;functions.ExceptionClear=mock::ClearException;
    functions.NewGlobalRef=mock::Global;functions.DeleteGlobalRef=mock::DeleteGlobal;functions.DeleteLocalRef=mock::DeleteLocal;
    functions.CallStaticObjectMethodV=mock::StaticObject;functions.GetObjectField=mock::GetField;
    functions.CallBooleanMethodV=mock::BoolMethod;functions.GetBooleanField=mock::BoolField;
    functions.IsInstanceOf=mock::Instance;functions.IsSameObject=mock::Same;functions.NewString=mock::NewString;
    functions.CallVoidMethodV=mock::VoidMethod;functions.CallObjectMethodV=mock::ObjectMethod;functions.GetStringLength=mock::Length;
    JNIEnv env{&functions};
    for(bool legacy:{true,false}) {
        mock::legacy=legacy;mock::focused=mock::enabled=mock::chat=mock::clientThread=true;
        mock::screen=mock::Id<jobject>(10);mock::world=mock::Id<jobject>(20);
        nova::module::imefix::ChatInput adapter;
        assert(adapter.Resolve(&env) && adapter.OnClientThread(&env));assert(mock::globals==3);
        bool changed=false;assert(adapter.RefreshTarget(&env,changed)&&changed&&mock::globals==6);
        assert(adapter.RefreshTarget(&env,changed)&&!changed);
        const int before=mock::insertions;
        assert(adapter.Insert(&env,u"中文测试你好"));assert(mock::committed==u"中文测试你好"&&mock::insertions==before+1);
        mock::clientThread=false;assert(!adapter.OnClientThread(&env));mock::clientThread=true;
        mock::focused=false;assert(!adapter.RefreshTarget(&env,changed)&&changed);
        assert(!adapter.Insert(&env,u"旧聊天框"));mock::focused=true;
        assert(adapter.RefreshTarget(&env,changed));
        mock::screen=mock::Id<jobject>(11);assert(adapter.RefreshTarget(&env,changed)&&changed&&mock::globals==6);
        mock::world=nullptr;assert(!adapter.RefreshTarget(&env,changed)&&changed&&mock::globals==3);
        mock::world=mock::Id<jobject>(20);mock::chat=false;assert(!adapter.RefreshTarget(&env,changed));mock::chat=true;
        if(legacy){mock::enabled=false;assert(!adapter.RefreshTarget(&env,changed));mock::enabled=true;}
        assert(adapter.RefreshTarget(&env,changed));
        mock::failInsert=true;assert(adapter.Insert(&env,u"中文"));assert(!mock::exception);mock::failInsert=false;
        adapter.Release(&env);assert(mock::globals==0);
        mock::missing=true;assert(!adapter.Resolve(&env));adapter.Release(&env);assert(mock::globals==0&&!mock::exception);mock::missing=false;
    }
    std::puts("PASS: real legacy/modern chat adapter bindings, UTF-16 insertion, focus/enabled/thread/world gates, responder ownership and JNI release");
}
