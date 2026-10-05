#include "minecraft/hooks/Transformer.h"

#include "minecraft/hooks/Edits.h"
#include "minecraft/java/api/class_edit.h"
#include "minecraft/java/api/hook_bridge.h"
#include "minecraft/compat/Profile.h"

#include <atomic>
#include <cstring>
#include <string>
#include <vector>

namespace nova::hook {
namespace {

std::atomic<int> g_transformed{0};
std::atomic<bool> g_enabled{false};
std::atomic<bool> g_rewriteFailed{false};

bool LoadRequiredTargets(JNIEnv* env) {
    // Target classes can be lazy in every profile: e.g. GuiConnecting$1 at
    // the Lunar 1.8 main menu, and FirstPersonHandsAndItems in 26.3. Load the
    // entire final rule table before taking the loaded-class snapshot. Using
    // the game's loadClass avoids class initialization and starting a join.
    jobject loader = compat::ClassLoader(env);
    if (!loader) {
        LogTo("Transformer: target loading failed: game class loader unavailable");
        return false;
    }
    int count = 0;
    const auto* edits = AllEdits(count);
    bool complete = true;
    for (int i = 0; i < count; ++i) {
        std::string name(edits[i].internalName);
        for (auto& c : name) if (c == '/') c = '.';
        jclass target = LoadClassInLoader(env, loader, name.c_str());
        const bool exception = env->ExceptionCheck();
        if (exception) env->ExceptionClear();
        if (!target || exception) {
            LogTo("Transformer: required target could not be loaded: %s", edits[i].internalName);
            complete = false;
        }
        if (target) env->DeleteLocalRef(target);
    }
    env->DeleteGlobalRef(loader);
    return complete;
}

bool IsOurOwnClass(const char* internalName) {
    const std::string& bridge = nova::bridge::InternalName();
    return !bridge.empty() && bridge == internalName;
}

bool Rewrite(const char* internalName,
             const unsigned char* data, size_t length,
             std::vector<u1>& out) {
    const ClassEdits* edits = EditsFor(internalName);
    if (!edits || (edits->count <= 0 && edits->insertionCount <= 0 &&
                   edits->guardCount <= 0 && edits->appendCount <= 0))
        return false;

    ClassFileEditor editor(data, length);
    const std::string& bridge = nova::bridge::InternalName();
    int applied = 0;

    for (int i = 0; i < edits->count; ++i) {
        const Redirect& redirect = edits->redirects[i];
        if (editor.redirectCalls(redirect.method, redirect.methodDesc,
                                 redirect.calleeOwner, redirect.calleeName,
                                 redirect.calleeDesc, bridge,
                                 redirect.hook, redirect.hookDesc, redirect.opcode,
                                 redirect.limit, redirect.ordinal)) {
            ++applied;
            continue;
        }

        LogTo("Transformer: %s.%s matched no call site; the rule was skipped",
              internalName, redirect.method);
    }

    for (int i = 0; i < edits->insertionCount; ++i) {
        const Insertion& insertion = edits->insertions[i];
        if (editor.insertHookCall(insertion.method, insertion.methodDesc,
                                  insertion.calleeOwner, insertion.calleeName,
                                  insertion.calleeDesc, insertion.calleeIsStatic,
                                  insertion.ordinal, bridge,
                                  insertion.hook, insertion.hookDesc,
                                  insertion.argCount, insertion.storeSlot)) {
            ++applied;
            continue;
        }
        LogTo("Transformer: %s.%s could not take the %s insertion; skipped",
              internalName, insertion.method, insertion.hook);
    }

    for (int i = 0; i < edits->guardCount; ++i) {
        const Guard& guard = edits->guards[i];
        if (editor.insertCancelGuard(guard.method, guard.methodDesc, bridge,
                                     guard.hook, guard.hookDesc, guard.argCount, guard.entryReturn)) {
            ++applied;
            continue;
        }
        LogTo("Transformer: %s.%s could not take the %s guard; skipped",
              internalName, guard.method, guard.hook);
    }

    for (int i = 0; i < edits->appendCount; ++i) {
        const Append& append = edits->appends[i];
        const u2 ref = editor.addMethodRef(bridge, append.hook, append.hookDesc);
        if (ref == 0) {
            LogTo("Transformer: %s.%s could not reference the %s hook; skipped",
                  internalName, append.method, append.hook);
            continue;
        }

        const std::vector<u1> code =
            AppendCode(ref, append.loads, append.loadCount);
        const u2 depth = static_cast<u2>(append.loadCount);
        const bool placed = append.atHead
            ? editor.prependToMethodCode(append.method, append.methodDesc, code, depth)
            : append.allValueReturns
            ? editor.appendBeforeValueReturns(append.method, append.methodDesc, code, depth)
            : append.allVoidReturns
            ? editor.appendBeforeVoidReturns(append.method, append.methodDesc, code, depth)
            : editor.appendBeforeLastReturn(append.method, append.methodDesc, code, depth);
        if (placed) {
            ++applied;
            continue;
        }
        LogTo("Transformer: %s.%s could not take the %s append; skipped",
              internalName, append.method, append.hook);
    }

    if (applied == 0 || ((compat::IsLegacyLunar() || compat::UsesModernRuntime()) && applied !=
        edits->count + edits->insertionCount + edits->guardCount + edits->appendCount)) {
        g_rewriteFailed.store(true, std::memory_order_release);
        return false;
    }

    out = editor.serialize();
    return !out.empty();
}

void DumpBytes(const char* stage, const char* internalName,
               const unsigned char* data, size_t length) {
    char dir[MAX_PATH];
    const DWORD n = GetTempPathA(MAX_PATH, dir);
    if (n == 0 || n >= MAX_PATH - 32) return;
    lstrcatA(dir, "nova-hook-");
    lstrcatA(dir, stage);
    CreateDirectoryA(dir, nullptr);

    std::string file(internalName);
    for (char& c : file) if (c == '/') c = '.';
    std::string path = std::string(dir) + "\\" + file + ".class";

    HANDLE h = CreateFileA(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    WriteFile(h, data, static_cast<DWORD>(length), &written, NULL);
    CloseHandle(h);
}

void JNICALL OnClassFileLoad(jvmtiEnv* jvmti, JNIEnv* env,
                             jclass classBeingRedefined, jobject loader,
                             const char* name, jobject protectionDomain,
                             jint classDataLen, const unsigned char* classData,
                             jint* newClassDataLen, unsigned char** newClassData) {
    (void)env; (void)classBeingRedefined; (void)loader; (void)protectionDomain;

    if (!name || !classData || classDataLen <= 0) return;
    if (newClassDataLen) *newClassDataLen = 0;
    if (newClassData) *newClassData = nullptr;
    if (IsOurOwnClass(name)) return;

    if (EditsFor(name) != nullptr)
        DumpBytes("in", name, classData, static_cast<size_t>(classDataLen));

    try {
        std::vector<u1> rewritten;
        if (!Rewrite(name, classData, static_cast<size_t>(classDataLen), rewritten))
            return;

        DumpBytes("out", name, rewritten.data(), rewritten.size());

        unsigned char* buffer = nullptr;
        if (jvmti->Allocate(static_cast<jlong>(rewritten.size()), &buffer) != JVMTI_ERROR_NONE)
            return;
        std::memcpy(buffer, rewritten.data(), rewritten.size());

        if (newClassData) *newClassData = buffer;
        if (newClassDataLen) *newClassDataLen = static_cast<jint>(rewritten.size());
        g_transformed.fetch_add(1, std::memory_order_relaxed);
    } catch (...) {
        g_rewriteFailed.store(true, std::memory_order_release);
        if (newClassDataLen) *newClassDataLen = 0;
        if (newClassData) *newClassData = nullptr;
    }
}

int RetransformLoaded(jvmtiEnv* jvmti, JNIEnv* env, bool* ok = nullptr) {
    if (ok) *ok = true;
    jint classCount = 0;
    jclass* classes = nullptr;
    const jvmtiError listed = jvmti->GetLoadedClasses(&classCount, &classes);
    if (listed != JVMTI_ERROR_NONE || !classes) {
        LogTo("Transformer: the loaded classes could not be listed (JVMTI error %d)", listed);
        if (ok) *ok = false;
        return 0;
    }

    std::vector<jclass> targets;
    std::vector<std::string> names;
    for (jint i = 0; i < classCount; ++i) {
        char* signature = nullptr;
        bool wanted = false;
        std::string internal;

        if (jvmti->GetClassSignature(classes[i], &signature, nullptr) == JVMTI_ERROR_NONE &&
            signature) {
            const size_t len = std::strlen(signature);
            if (len >= 3 && signature[0] == 'L' && signature[len - 1] == ';') {
                internal.assign(signature + 1, len - 2);
                wanted = EditsFor(internal.c_str()) != nullptr;
            }
            jvmti->Deallocate(reinterpret_cast<unsigned char*>(signature));
        }


        if (wanted) {
            targets.push_back(classes[i]);
            names.push_back(internal);
        } else {
            env->DeleteLocalRef(classes[i]);
        }
    }
    jvmti->Deallocate(reinterpret_cast<unsigned char*>(classes));

    // One call per class rather than one call for all of them. Retransform is
    // all-or-nothing per call, so a single class the JVM refuses would take the
    // other targets down with it -- and the refusal names the whole batch, not
    // the class that caused it.
    int done = 0;
    for (size_t i = 0; i < targets.size(); ++i) {
        const jvmtiError result = jvmti->RetransformClasses(1, &targets[i]);
        if (result == JVMTI_ERROR_NONE) {
            ++done;
            continue;
        }
        LogTo("Transformer: %s was not retransformed (JVMTI error %d)",
              names[i].c_str(), result);
        if (ok) *ok = false;
    }

    for (jclass c : targets) env->DeleteLocalRef(c);

    if (done != static_cast<int>(targets.size())) {
        LogTo("Transformer: %d of %zu classes took the edits", done, targets.size());
        return 0;
    }
    return done;
}

}

int TransformedCount() { return g_transformed.load(std::memory_order_relaxed); }

bool WaitForQuiescence(jvmtiEnv* jvmti, JNIEnv* env, unsigned timeoutMs) {
    if (!jvmti || !env) return false;
    const ULONGLONG deadline = GetTickCount64() + timeoutMs;
    do {
        jvmtiStackInfo* stacks = nullptr;
        jint count = 0;
        if (jvmti->GetAllStackTraces(1024, &stacks, &count) != JVMTI_ERROR_NONE || !stacks)
            return false;
        bool busy = false;
        for (jint i = 0; i < count; ++i) {
            if (stacks[i].frame_count >= 1024) busy = true;
            for (jint f = 0; f < stacks[i].frame_count && !busy; ++f) {
                const jmethodID method = stacks[i].frame_buffer[f].method;
                jboolean obsolete = JNI_FALSE;
                if (jvmti->IsMethodObsolete(method, &obsolete) != JVMTI_ERROR_NONE) { busy = true; break; }
                jclass declaring = nullptr;
                if (jvmti->GetMethodDeclaringClass(method, &declaring) != JVMTI_ERROR_NONE || !declaring) {
                    busy = true; break;
                }
                char* signature = nullptr;
                if (jvmti->GetClassSignature(declaring, &signature, nullptr) != JVMTI_ERROR_NONE || !signature) {
                    busy = true;
                } else {
                    const size_t length = std::strlen(signature);
                    if (length > 2 && signature[0] == 'L' && signature[length - 1] == ';') {
                        std::string name(signature + 1, length - 2);
                        busy = IsOurOwnClass(name.c_str()) || (obsolete && EditsFor(name.c_str()));
                    }
                }
                if (signature) jvmti->Deallocate(reinterpret_cast<unsigned char*>(signature));
                env->DeleteLocalRef(declaring);
            }
            if (stacks[i].thread) env->DeleteLocalRef(stacks[i].thread);
        }
        jvmti->Deallocate(reinterpret_cast<unsigned char*>(stacks));
        if (!busy) return true;
        Sleep(20);
    } while (GetTickCount64() < deadline);
    LogTo("Transformer: obsolete Java frames or bridge callbacks are still active");
    return false;
}

bool EnableTransformer(jvmtiEnv* jvmti, JNIEnv* env) {
    if (!jvmti || !env) return false;
    if (g_enabled.load(std::memory_order_acquire)) return true;

    if (!LoadRequiredTargets(env)) return false;

    jvmtiCapabilities capabilities{};
    capabilities.can_retransform_classes = 1;
    const jvmtiError capability = jvmti->AddCapabilities(&capabilities);
    if (capability != JVMTI_ERROR_NONE) {
        LogTo("Transformer: can_retransform_classes was refused (JVMTI error %d)",
              capability);
        return false;
    }

    jvmtiEventCallbacks callbacks{};
    callbacks.ClassFileLoadHook = &OnClassFileLoad;
    const jvmtiError registered = jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks));
    if (registered != JVMTI_ERROR_NONE) {
        LogTo("Transformer: the class file hook could not be registered (JVMTI error %d)",
              registered);
        return false;
    }

    const jvmtiError armed = jvmti->SetEventNotificationMode(
        JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, nullptr);
    if (armed != JVMTI_ERROR_NONE) {
        LogTo("Transformer: the class file hook could not be enabled (JVMTI error %d)",
              armed);
        return false;
    }

    g_enabled.store(true, std::memory_order_release);

    g_rewriteFailed.store(false, std::memory_order_release);
    bool complete = false;
    const int before = TransformedCount();
    const int retransformed = RetransformLoaded(jvmti, env, &complete);
    LogTo("Transformer: on; %d class%s retransformed, %d rewritten in total",
          retransformed, retransformed == 1 ? "" : "es", TransformedCount());
    if ((compat::IsLegacyLunar() || compat::UsesModernRuntime())) {
        int expected = 0;
        AllEdits(expected);
        const int rewritten = TransformedCount() - before;
        const bool valid = complete && retransformed == expected &&
            rewritten == expected && !g_rewriteFailed.load(std::memory_order_acquire);
        if (!valid)
            LogTo("Transformer: incomplete installation: expected=%d retransformed=%d rewritten=%d complete=%d rewriteFailed=%d",
                  expected, retransformed, rewritten, complete, g_rewriteFailed.load());
        return valid;
    }
    return true;
}

bool DisableTransformer(jvmtiEnv* jvmti, JNIEnv* env) {
    if (!g_enabled.exchange(false, std::memory_order_acq_rel)) return true;
    if (!jvmti || !env) return false;

    const jvmtiError disarmed = jvmti->SetEventNotificationMode(
        JVMTI_DISABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, nullptr);
    if (disarmed != JVMTI_ERROR_NONE) {
        LogTo("Transformer: the class file hook could not be disabled (JVMTI error %d)",
              disarmed);
    }

    jvmtiEventCallbacks none{};
    jvmti->SetEventCallbacks(&none, sizeof(none));

    bool complete = false;
    const int restored = RetransformLoaded(jvmti, env, &complete);
    LogTo("Transformer: off; %d class%s put back%s", restored,
          restored == 1 ? "" : "es",
          complete ? "" : " -- the JVM refused, so they are still edited");
    return complete;
}

}
