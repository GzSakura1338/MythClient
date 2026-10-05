#include "minecraft/compat/lunar12111/Lunar12111.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
namespace nova::compat::lunar12111 {
namespace { jfieldID screen{}; }
bool Initialize(JNIEnv* env) {
    using namespace mc263;
    access::Frame f(env);
    auto version=f.staticCall(names::SharedConstants_getCurrentVersion);
    const auto id=f.text(f.call(version,names::WorldVersion_id));
    if(id!="1.21.11") { LogTo("Lunar12111: expected 1.21.11, found %s",id.c_str());return false; }
    screen=access::Field(env,{"net.minecraft.client.Minecraft","screen","Lnet/minecraft/client/gui/screens/Screen;"});
    const auto handle=f.call<jlong>(f.call(f.client(),names::Minecraft_getWindow),names::Window_handle);
    LogTo("Lunar12111: intermediary namespace, Java 21, GLFW window=%p",reinterpret_cast<void*>(handle));
    return screen&&handle&&f.check();
}
jobject Screen(JNIEnv* env,jobject client) {return client&&screen?env->GetObjectField(client,screen):nullptr;}
void Shutdown(JNIEnv*) {screen=nullptr;}
}
