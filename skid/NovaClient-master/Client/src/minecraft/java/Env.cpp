#include "minecraft/java/api/base.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar12111/Mapping.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <atomic>

JavaVM*   g_vm    = nullptr;
jvmtiEnv* g_jvmti = nullptr;

namespace {
#define NOVA_FORGE1201_ENV
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_ENV
}

jvmtiEnv* CurrentJvmti() {
    // Unlike JNIEnv, a JVMTI environment is shared across attached threads.
    // GetEnv(JVMTI_VERSION_1_2) CREATES an environment on every call. Repeating
    // it from lookup helpers leaks VM-owned environments until process exit.
    // RuntimeThread owns this environment and drains callbacks before disposal.
    return g_jvmti;
}

bool DisposeJvmti() {
    if (!g_jvmti) return true;
    const jvmtiError result = g_jvmti->DisposeEnvironment();
    if (result != JVMTI_ERROR_NONE) {
        LogTo("Base: JVMTI environment disposal failed (%d); DLL retained", result);
        return false;
    }
    g_jvmti = nullptr;
    return true;
}

void LogAndClearException(JNIEnv* env, const char* where) {
    if (!env || !env->ExceptionCheck()) return;
    jthrowable t = env->ExceptionOccurred();
    env->ExceptionClear();

    jclass thrCls = env->FindClass("java/lang/Throwable");
    jmethodID toStringMid =
        env->GetMethodID(thrCls, "toString", "()Ljava/lang/String;");
    jstring msg = (jstring)env->CallObjectMethod(t, toStringMid);

    const char* c = msg ? env->GetStringUTFChars(msg, nullptr) : "<unknown>";
    Dbg("[%s] %s", where, c);
    if (msg) env->ReleaseStringUTFChars(msg, c);

    jmethodID pst = env->GetMethodID(thrCls, "printStackTrace", "()V");
    if (pst) env->CallVoidMethod(t, pst);
    if (env->ExceptionCheck()) env->ExceptionClear();

    env->DeleteLocalRef(t);
    env->DeleteLocalRef(thrCls);
    if (msg) env->DeleteLocalRef(msg);
}

JniAttach::JniAttach() {
    if (!g_vm) return;
    jint rc = g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_8);
    if (rc == JNI_EDETACHED) {
        JavaVMAttachArgs args{};
        args.version = JNI_VERSION_1_8;
        args.name = const_cast<char*>("NovaBaseNative");
        args.group = nullptr;
        if (g_vm->AttachCurrentThreadAsDaemon(
                reinterpret_cast<void**>(&env), &args) == JNI_OK) {
            attached = true;
        } else {
            env = nullptr;
        }
    } else if (rc != JNI_OK) {
        env = nullptr;
    }
}

JniAttach::~JniAttach() {
    if (attached && g_vm) g_vm->DetachCurrentThread();
}

jobject GetMinecraftClassLoader(JNIEnv* env) {
    if (env) {
        if (jobject selected = nova::compat::ClassLoader(env)) return selected;
    }
    jvmtiEnv* jvmti = CurrentJvmti();
    if (!env || !jvmti) return nullptr;

    jint count = 0;
    jthread* threads = nullptr;
    if (jvmti->GetAllThreads(&count, &threads) != JVMTI_ERROR_NONE) {
        return nullptr;
    }

    static int s_dump_count = 0;
    bool dump = (s_dump_count++ < 3);
    if (dump) LogTo("GetMinecraftClassLoader: enumerating %d threads", (int)count);

    jobject result = nullptr;
    for (jint i = 0; i < count; ++i) {
        jvmtiThreadInfo info{};
        if (jvmti->GetThreadInfo(threads[i], &info) != JVMTI_ERROR_NONE) continue;

        bool hasLoader = (info.context_class_loader != nullptr);
        if (dump) {
            LogTo("  thread[%d] name='%s' has_loader=%d",
                  (int)i,
                  info.name ? info.name : "(null)",
                  hasLoader ? 1 : 0);
        }

        bool hit = info.name && hasLoader &&
                   std::strcmp(info.name, "Render thread") == 0;

        if (!hit && info.name && hasLoader) {
            jclass conn = LoadClassInLoader(env, info.context_class_loader,
                                            kConnectionDot);
            if (env->ExceptionCheck()) env->ExceptionClear();
            if (conn) {
                if (dump) LogTo("     -> can load MC.Connection, PICKED");
                hit = true;
                env->DeleteLocalRef(conn);
            }
        }

        if (hit) result = env->NewGlobalRef(info.context_class_loader);

        if (info.context_class_loader) env->DeleteLocalRef(info.context_class_loader);
        if (info.thread_group) env->DeleteLocalRef(info.thread_group);
        if (info.name) jvmti->Deallocate(reinterpret_cast<unsigned char*>(info.name));
        if (result) break;
    }

    for (jint i = 0; i < count; ++i) env->DeleteLocalRef(threads[i]);
    jvmti->Deallocate(reinterpret_cast<unsigned char*>(threads));
    return result;
}

jclass LoadClassInLoader(JNIEnv* env, jobject classLoader, const char* dotName) {
    if (!env || !classLoader || !dotName) return nullptr;
    if (nova::compat::IsLunar12111()) dotName=nova::compat::lunar12111::ClassName(dotName);

    jclass clCls = env->GetObjectClass(classLoader);
    jmethodID mid =
        env->GetMethodID(clCls, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    if (!mid) {
        env->ExceptionClear();
        env->DeleteLocalRef(clCls);
        return nullptr;
    }

    jstring jname = env->NewStringUTF(dotName);
    jclass  result = (jclass)env->CallObjectMethod(classLoader, mid, jname);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        result = nullptr;
    }

    env->DeleteLocalRef(jname);
    env->DeleteLocalRef(clCls);
    return result;
}
