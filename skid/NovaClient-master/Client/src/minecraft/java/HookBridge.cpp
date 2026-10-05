#include "minecraft/java/api/hook_bridge.h"

#include "minecraft/hooks/Edits.h"
#include "minecraft/hooks/Hooks.h"
#include "minecraft/java/api/class_edit.h"
#include "minecraft/java/api/classfile.h"
#include "minecraft/java/api/random_name.h"

#include <cstring>

namespace nova::bridge {
namespace {

std::string g_package;
std::string g_internal;
std::string g_dot;

jclass g_class = nullptr;

bool g_installed = false;
bool g_attempted = false;

bool DescriptorsAreWellFormed() {
    bool all = true;
    for (int i = 0; i < HookCount(); ++i) {
        const HookDef& hook = Hooks()[i];
        int popped = 0, pushed = 0;
        if (!ClassFileEditor::stackEffect(hook.descriptor, true,
                                          popped, pushed)) {
            LogTo("Bridge: hook %s has a descriptor that does not parse: %s",
                  hook.name, hook.descriptor);
            all = false;
        }
    }
    return all;
}

bool EditsAgreeWithHooks() {
    int classCount = 0;
    const nova::hook::ClassEdits* edits = nova::hook::AllEdits(classCount);
    bool all = true;

    auto agrees = [&](const char* internalName, const char* method,
                      const char* hookName, const char* hookDesc) {
        const HookDef* found = nullptr;
        for (int i = 0; i < HookCount(); ++i) {
            if (std::strcmp(Hooks()[i].name, hookName) == 0) {
                found = &Hooks()[i];
                break;
            }
        }

        if (!found) {
            LogTo("Bridge: %s.%s points at %s, which is not a hook",
                  internalName, method, hookName);
            return false;
        }
        if (std::strcmp(found->descriptor, hookDesc) != 0) {
            LogTo("Bridge: %s.%s points at %s%s, but the hook is declared %s",
                  internalName, method, hookName, hookDesc, found->descriptor);
            return false;
        }
        return true;
    };

    for (int c = 0; c < classCount; ++c) {
        const nova::hook::ClassEdits& edited = edits[c];
        for (int r = 0; r < edited.count; ++r) {
            const nova::hook::Redirect& redirect = edited.redirects[r];
            all &= agrees(edited.internalName, redirect.method,
                          redirect.hook, redirect.hookDesc);
        }
        for (int i = 0; i < edited.insertionCount; ++i) {
            const nova::hook::Insertion& insertion = edited.insertions[i];
            all &= agrees(edited.internalName, insertion.method,
                          insertion.hook, insertion.hookDesc);
        }
        for (int i = 0; i < edited.guardCount; ++i) {
            const nova::hook::Guard& guard = edited.guards[i];
            all &= agrees(edited.internalName, guard.method,
                          guard.hook, guard.hookDesc);
        }
        for (int i = 0; i < edited.appendCount; ++i) {
            const nova::hook::Append& append = edited.appends[i];
            all &= agrees(edited.internalName, append.method, append.hook, append.hookDesc);
        }
    }

    return all;
}

std::vector<u1> BuildClass() {
    ClassBuilder builder(g_internal, "java/lang/Object");

    for (int i = 0; i < HookCount(); ++i) {
        const HookDef& hook = Hooks()[i];
        builder.addNativeMethod(hook.name, hook.descriptor);
    }

    return builder.build();
}

}

const std::string& InternalName() { return g_internal; }
const std::string& DotName() { return g_dot; }
bool IsInstalled() { return g_installed; }

bool Install(JNIEnv* env, jobject mcLoader) {
    if (g_installed) return true;
    if (g_attempted) return false;

    if (!env || !mcLoader) {
        LogTo("Bridge: no class loader to define into");
        return false;
    }

    const int count = HookCount();
    if (count <= 0) {
        LogTo("Bridge: no hooks to install");
        return false;
    }

    g_package = GetTrampolinePackage();
    g_internal = MakeInternalName(g_package, GenerateRandomClassName(2, 3));
    g_dot = g_internal;
    for (char& c : g_dot) {
        if (c == '/') c = '.';
    }

    if (!DescriptorsAreWellFormed()) return false;
    if (!EditsAgreeWithHooks()) return false;

    if (!nova::hook::Prepare(env, mcLoader)) {
        LogTo("Bridge: the hooks could not resolve what they stand in for");
        return false;
    }

    const std::vector<u1> bytes = BuildClass();

    g_attempted = true;

    jclass defined = env->DefineClass(
        g_internal.c_str(), mcLoader,
        reinterpret_cast<const jbyte*>(bytes.data()),
        static_cast<jint>(bytes.size()));

    if (!defined || env->ExceptionCheck()) {
        LogTo("Bridge: DefineClass(%s) failed for %zu bytes",
              g_internal.c_str(), bytes.size());
        if (env->ExceptionCheck()) env->ExceptionClear();
        return false;
    }

    std::vector<JNINativeMethod> natives;
    natives.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        const HookDef& hook = Hooks()[i];
        JNINativeMethod method{};
        method.name = const_cast<char*>(hook.name);
        method.signature = const_cast<char*>(hook.descriptor);
        method.fnPtr = hook.impl;
        natives.push_back(method);
    }

    if (env->RegisterNatives(defined, natives.data(),
                             static_cast<jint>(natives.size())) != JNI_OK) {
        LogTo("Bridge: RegisterNatives(%s) failed for %d hook%s",
              g_internal.c_str(), count, count == 1 ? "" : "s");
        if (env->ExceptionCheck()) env->ExceptionClear();
        return false;
    }

    g_class = static_cast<jclass>(env->NewGlobalRef(defined));
    if (!g_class) {
        env->UnregisterNatives(defined);
        env->DeleteLocalRef(defined);
        LogTo("Bridge: the generated class could not be kept");
        return false;
    }
    env->DeleteLocalRef(defined);

    g_installed = true;
    LogTo("Bridge: %d hook%s bound to %s", count, count == 1 ? "" : "s", g_dot.c_str());
    return true;
}

bool Release(JNIEnv* env) {
    if (!env) return false;
    if (g_class) {
        if (env->UnregisterNatives(g_class) != JNI_OK || env->ExceptionCheck()) {
            if (env->ExceptionCheck()) env->ExceptionClear();
            LogTo("Bridge: native methods could not be unregistered");
            return false;
        }
        env->DeleteGlobalRef(g_class);
        g_class = nullptr;
    }
    nova::hook::Release(env);
    g_installed = false;
    return true;
}

}
