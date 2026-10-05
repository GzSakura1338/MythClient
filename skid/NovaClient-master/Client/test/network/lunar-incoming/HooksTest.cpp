#define NOMINMAX
#include <Windows.h>
#include "jni.h"
#include "minecraft/java/api/class_edit.h"
#include "minecraft/hooks/Edits.h"
#include "../../bytecode/audit.h"
#include <fstream>
#include <iostream>
#include <filesystem>

namespace {
bool cancelTask = true;
int preprocessed = 0, processed = 0;
jboolean JNICALL Guard(JNIEnv*, jclass, jobject) { return cancelTask; }
void JNICALL Preprocess(JNIEnv*, jclass, jobject) { ++preprocessed; }
void JNICALL Process(JNIEnv*, jobject, jobject) { ++processed; }
void Require(JNIEnv* env, bool ok, const char* operation) {
    if (env->ExceptionCheck()) { env->ExceptionDescribe(); ok = false; }
    if (!ok) { std::cerr << "FAIL: " << operation << '\n'; std::exit(10); }
}
jclass Define(JNIEnv* env, ClassBuilder& builder) {
    auto bytes = builder.build();
    auto c = env->DefineClass(builder.internalName().c_str(), nullptr,
                             reinterpret_cast<const jbyte*>(bytes.data()), static_cast<jsize>(bytes.size()));
    Require(env, c != nullptr, builder.internalName().c_str()); return c;
}
void VerifyInJvm(const std::vector<u1>& rewritten, const char* dll) {
    const auto bin = std::filesystem::path(dll).parent_path().parent_path();
    SetDllDirectoryW(bin.c_str());
    auto library = LoadLibraryW(std::filesystem::path(dll).c_str());
    if (!library) { std::cerr << "Cannot load test JVM: " << GetLastError(); std::exit(11); }
    using Create = jint(JNICALL*)(JavaVM**, void**, void*);
    const auto create = reinterpret_cast<Create>(GetProcAddress(library, "JNI_CreateJavaVM"));
    JavaVMOption options[] = {{const_cast<char*>("-Xverify:all"), nullptr},
                              {const_cast<char*>("-Xmx64m"), nullptr}};
    JavaVMInitArgs args{JNI_VERSION_1_8, 2, options, JNI_FALSE};
    JavaVM* vm = nullptr; JNIEnv* env = nullptr;
    if (!create || create(&vm, reinterpret_cast<void**>(&env), &args) != JNI_OK) std::exit(12);
    // Dependencies are minimal classfiles generated in C++, with no Java
    // source or attached game. The Runnable under test is the real Lunar one.
    ClassBuilder packet("net/minecraft/network/Packet", "java/lang/Object");
    packet.setAccessFlags(ACC_PUBLIC | ACC_ABSTRACT | 0x0200);
    packet.addCodedMethod("processPacket", "(Lnet/minecraft/network/INetHandler;)V", ACC_PUBLIC, {0xB1}, 0, 2);
    ClassBuilder listener("net/minecraft/network/INetHandler", "java/lang/Object");
    listener.setAccessFlags(ACC_PUBLIC | ACC_ABSTRACT | 0x0200);
    Define(env, listener); Define(env, packet);
    ClassBuilder util("net/minecraft/network/PacketThreadUtil", "java/lang/Object");
    util.addNativeMethod("clientPreProcessPacket", "(Lnet/minecraft/network/Packet;)V");
    const auto utilClass = Define(env, util);
    JNINativeMethod pre{const_cast<char*>("clientPreProcessPacket"),
        const_cast<char*>("(Lnet/minecraft/network/Packet;)V"), reinterpret_cast<void*>(&Preprocess)};
    Require(env, env->RegisterNatives(utilClass, &pre, 1) == JNI_OK, "register preprocessing");
    ClassBuilder bridge("nova/test/NetworkBridge", "java/lang/Object");
    bridge.addNativeMethod("clientPacketTask", "(Ljava/lang/Object;)Z");
    const auto bridgeClass = Define(env, bridge);
    JNINativeMethod guard{const_cast<char*>("clientPacketTask"), const_cast<char*>("(Ljava/lang/Object;)Z"), reinterpret_cast<void*>(&Guard)};
    Require(env, env->RegisterNatives(bridgeClass, &guard, 1) == JNI_OK, "register guard");

    ClassBuilder concrete("nova/test/TestPacket", "java/lang/Object");
    const auto packetInterface = concrete.classRef("net/minecraft/network/Packet");
    concrete.addNativeMethod("processPacket", "(Lnet/minecraft/network/INetHandler;)V", ACC_PUBLIC);
    auto concreteBytes = concrete.build();
    const u1* cursor = concreteBytes.data() + 8;
    audit::Pool cp;
    if (!audit::ParsePool(cursor, concreteBytes.data() + concreteBytes.size(), cp)) std::exit(13);
    const size_t interfaceCount = cursor - concreteBytes.data() + 6;
    concreteBytes[interfaceCount + 1] = 1;
    concreteBytes.insert(concreteBytes.begin() + interfaceCount + 2,
                         {u1(packetInterface >> 8), u1(packetInterface)});
    const auto concreteClass = env->DefineClass("nova/test/TestPacket", nullptr,
        reinterpret_cast<const jbyte*>(concreteBytes.data()), static_cast<jsize>(concreteBytes.size()));
    Require(env, concreteClass != nullptr, "define concrete packet");
    JNINativeMethod process{const_cast<char*>("processPacket"), const_cast<char*>("(Lnet/minecraft/network/INetHandler;)V"), reinterpret_cast<void*>(&Process)};
    Require(env, env->RegisterNatives(concreteClass, &process, 1) == JNI_OK, "register packet handler");
    const auto value = env->AllocObject(concreteClass);
    const auto task = env->DefineClass("net/minecraft/network/PacketThreadUtil$1", nullptr,
        reinterpret_cast<const jbyte*>(rewritten.data()), static_cast<jsize>(rewritten.size()));
    Require(env, task != nullptr, "define rewritten real Runnable");
    const auto ctor = env->GetMethodID(task, "<init>", "(Lnet/minecraft/network/Packet;Lnet/minecraft/network/INetHandler;)V");
    const auto run = env->GetMethodID(task, "run", "()V");
    Require(env, ctor && run, "verify rewritten Runnable methods");
    const auto instance = env->NewObject(task, ctor, value, nullptr);
    Require(env, instance != nullptr, "construct rewritten Runnable");
    env->CallVoidMethod(instance, run);
    Require(env, preprocessed == 0 && processed == 0, "guard true skips original body");
    cancelTask = false;
    env->CallVoidMethod(instance, run);
    Require(env, preprocessed == 1 && processed == 1, "guard false preserves vanilla body exactly once");
    vm->DestroyJavaVM();
    std::cout << "PASS: isolated JVM -Xverify:all, real Lunar Runnable, true/false guard execution\n";
}
}

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    nova::hook::SelectLunarEdits(true);
    auto edits = nova::hook::EditsFor("net/minecraft/network/PacketThreadUtil$1");
    std::ifstream file(argv[1], std::ios::binary);
    std::vector<u1> bytes{std::istreambuf_iterator<char>(file), {}};
    if (!edits || edits->guardCount != 1 || bytes.empty()) return 3;
    ClassFileEditor editor(bytes.data(), bytes.size());
    const auto& guard = edits->guards[0];
    const auto original = editor.serialize();
    if (editor.insertCancelGuard(guard.method, guard.methodDesc, "nova/test/NetworkBridge",
                                  guard.hook, "()V", 1) || editor.serialize() != original) return 6;
    if (!editor.insertCancelGuard(guard.method, guard.methodDesc, "nova/test/NetworkBridge",
                                  guard.hook, guard.hookDesc, guard.argCount)) return 4;
    const auto result = editor.serialize();
    std::string error;
    if (!audit::AuditClass(result, error)) { std::cerr << error; return 5; }
    std::cout << "PASS: actual Lunar PacketThreadUtil$1 run guard and stack-map audit\n";
    VerifyInJvm(result, argv[2]);
}
