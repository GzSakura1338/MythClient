// Real JNI with a small Java fixture, not an injection into the user's Minecraft.
#include "minecraft/compat/minecraft263/Minecraft263.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/compat/minecraft263/Minecraft263Game.h"
#include <cstdarg>
#include <cstdio>
#include <limits>
#include <cstdlib>

// Entity adapters are outside this fixture. Fail immediately if a tested path
// accidentally reaches them, rather than silently providing fake entity behavior.
namespace nova::java {
EntityHandle::~EntityHandle(){if(raw())std::abort();}
EntityHandle& EntityHandle::operator=(EntityHandle&&) noexcept {std::abort();}
EntityHandle AdoptEntity(void*){std::abort();}
}

JavaVM* g_vm{};
jvmtiEnv* g_jvmti{};
static jobject loader{};
static jclass minecraft{};
static jmethodID instance{};
JniAttach::JniAttach() { if (g_vm) g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_8); }
JniAttach::~JniAttach() {}
void LogTo(const char* format, ...) {
    va_list args; va_start(args, format); vprintf(format, args); va_end(args); puts("");
}
jclass LoadClassInLoader(JNIEnv* env, jobject cl, const char* name) {
    jclass type = env->GetObjectClass(cl);
    auto load = env->GetMethodID(type, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    jstring text = env->NewStringUTF(name);
    auto result = static_cast<jclass>(env->CallObjectMethod(cl, load, text));
    if (env->ExceptionCheck()) { env->ExceptionClear(); result = nullptr; }
    env->DeleteLocalRef(text); env->DeleteLocalRef(type);
    return result;
}
namespace nova::compat {
jobject ClassLoader(JNIEnv* env) { return env->NewGlobalRef(loader); }
jobject LocalPlayer(JNIEnv* env) {
    auto mc=Minecraft(env);
    auto field=env->GetFieldID(minecraft,"player","Lnet/minecraft/client/player/LocalPlayer;");
    if(env->ExceptionCheck()||!field)return nullptr;
    auto result=env->GetObjectField(mc,field);env->DeleteLocalRef(mc);return result;
}
jobject Minecraft(JNIEnv* env) {
    jobject result = env->CallStaticObjectMethod(minecraft, instance);
    if (env->ExceptionCheck()) { env->ExceptionClear(); return nullptr; }
    return result;
}
bool ScreenOpen() {
    JniAttach attach;
    jobject mc = Minecraft(attach.env);
    jobject screen = mc263::Screen(attach.env, mc);
    const bool open = attach->ExceptionCheck() || screen;
    if (attach->ExceptionCheck()) attach->ExceptionClear();
    if (screen) attach->DeleteLocalRef(screen);
    attach->DeleteLocalRef(mc);
    return open;
}
}
extern "C" JNIEXPORT jstring JNICALL Java_Probe_init(JNIEnv* env, jclass) {
    env->GetJavaVM(&g_vm);
    jclass cl = env->FindClass("java/lang/ClassLoader");
    auto get = env->GetStaticMethodID(cl, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
    jobject local = env->CallStaticObjectMethod(cl, get);
    if (env->ExceptionCheck()) return nullptr;
    loader = env->NewGlobalRef(local);
    env->DeleteLocalRef(local); env->DeleteLocalRef(cl);
    jclass mc = env->FindClass("net/minecraft/client/Minecraft");
    minecraft = static_cast<jclass>(env->NewGlobalRef(mc));
    env->DeleteLocalRef(mc);
    instance = env->GetStaticMethodID(minecraft, "getInstance", "()Lnet/minecraft/client/Minecraft;");
    return nova::compat::mc263::Initialize(env) ? nullptr : env->NewStringUTF(nova::compat::mc263::Error());
}
extern "C" JNIEXPORT void JNICALL Java_Probe_shutdown(JNIEnv* env, jclass) {
    nova::compat::mc263::access::Release(env);
    nova::compat::mc263::Shutdown(env);
    env->DeleteGlobalRef(loader); env->DeleteGlobalRef(minecraft);
    loader = nullptr; minecraft = nullptr; instance = nullptr;
}
extern "C" JNIEXPORT jboolean JNICALL Java_Probe_motion(JNIEnv*,jclass,jobject packet,jint id,jint x,jint y,jint z){
    int actualId=0,ax=0,ay=0,az=0;
    return nova::compat::mc263::ReadMotionPacket(packet,actualId,ax,ay,az)&&actualId==id&&ax==x&&ay==y&&az==z;
}
extern "C" JNIEXPORT jobject JNICALL Java_Probe_posRot(JNIEnv*,jclass,jobject packet){
    nova::java::PosRot value;
    if(!nova::compat::mc263::ReadPosRot(packet,value))return nullptr;
    value.yaw+=10;return static_cast<jobject>(nova::compat::mc263::MakePosRot(value));
}
extern "C" JNIEXPORT jboolean JNICALL Java_Probe_click(JNIEnv*,jclass,jint id){
    return nova::compat::mc263::ClickMenuSlot(id,4,0,nova::java::ClickKind::QuickMove);
}
extern "C" JNIEXPORT jobject JNICALL Java_Probe_stack(JNIEnv*,jclass,jint index,jboolean armor){
    jobject stack=nullptr;
    nova::compat::mc263::InventoryStack(index,armor!=JNI_FALSE,stack);return stack;
}
extern "C" JNIEXPORT jboolean JNICALL Java_Probe_replay(JNIEnv*,jclass,jobject packet){return nova::compat::mc263::HandleIncomingPacket(packet);}
extern "C" JNIEXPORT jboolean JNICALL Java_Probe_mouse(JNIEnv*, jclass, jboolean capture) {
    return nova::compat::mc263::CaptureMouse(capture);
}
extern "C" JNIEXPORT jboolean JNICALL Java_Probe_sprint(JNIEnv*, jclass, jboolean down) {
    return nova::compat::mc263::SetSprintKeyDown(down);
}
extern "C" JNIEXPORT jdouble JNICALL Java_Probe_gamma(JNIEnv*, jclass) {
    double value{};
    return nova::compat::mc263::GetGamma(&value) ? value : std::numeric_limits<double>::quiet_NaN();
}
extern "C" JNIEXPORT jboolean JNICALL Java_Probe_setGamma(JNIEnv*, jclass, jdouble value) {
    return nova::compat::mc263::SetGamma(value);
}
extern "C" JNIEXPORT jlong JNICALL Java_Probe_window(JNIEnv*, jclass) {
    return reinterpret_cast<jlong>(nova::compat::mc263::WindowHandle());
}
extern "C" JNIEXPORT jboolean JNICALL Java_Probe_vulkanHandles(JNIEnv*, jclass) {
    nova::vkoverlay::Handles handles{};
    return nova::compat::mc263::IsVulkan() && nova::compat::mc263::VulkanHandles(&handles) &&
        handles.device == 11 && handles.physicalDevice == 22 && handles.instance == 33 &&
        handles.queue == 44 && handles.surface == 55 && handles.swapchain == 66 && handles.queueFamily == 2;
}
