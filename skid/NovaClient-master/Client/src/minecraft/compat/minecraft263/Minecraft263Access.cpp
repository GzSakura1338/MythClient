#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/compat/lunar1710/Mapping.h"
#include "minecraft/compat/easecation/EaseCation.h"
#include "minecraft/compat/neoforge12110/NeoForge12110.h"
#include <mutex>
#include <unordered_map>

namespace nova::compat::mc263::access {
namespace {
std::recursive_mutex mutex;
std::unordered_map<std::string, jclass> classes;
std::unordered_map<std::string, jmethodID> methods;
std::unordered_map<std::string, jfieldID> fields;
std::string Key(Member m, bool stat) {
    return std::string(m.owner) + "/" + m.name + m.descriptor + (stat ? "S" : "I");
}
bool Clean(JNIEnv* env) {
    if (!env->ExceptionCheck()) return true;
    env->ExceptionClear(); return false;
}
}
jclass Class(JNIEnv* env, const char* name) {
    if (!env || !name || !*name) return nullptr;
    name = easecation::ClassName(name);
    name = neoforge12110::ClassName(name);
    if (!name || !*name) return nullptr;
    std::lock_guard lock(mutex);
    if (auto it = classes.find(name); it != classes.end()) return it->second;
    auto loader = compat::ClassLoader(env);
    auto local = LoadClassInLoader(env, loader, name);
    if (loader) env->DeleteGlobalRef(loader);
    const bool clean = Clean(env);
    auto global = clean && local ? static_cast<jclass>(env->NewGlobalRef(local)) : nullptr;
    if (local) env->DeleteLocalRef(local);
    if (!global) LogTo("JNI access: missing class %s", name);
    classes.emplace(name, global);
    return global;
}
jmethodID Method(JNIEnv* env, Member m, bool stat) {
    // Validate before constructing the cache key or translating any names.
    if (!env || !ValidMember(m)) return nullptr;
    std::lock_guard lock(mutex);
    auto key = Key(m, stat);
    if (auto it = methods.find(key); it != methods.end()) return it->second;
    m = lunar1710::Translate(m);
    if (!ValidMember(m)) return nullptr;
    auto type = Class(env, m.owner);
    auto id = type ? (stat ? env->GetStaticMethodID(type, m.name, m.descriptor) : env->GetMethodID(type, m.name, m.descriptor)) : nullptr;
    if (!Clean(env)) id = nullptr;
    if (!id) LogTo("JNI access: missing method %s", key.c_str());
    methods.emplace(std::move(key), id); return id;
}
jfieldID Field(JNIEnv* env, Member m, bool stat) {
    if (!env || !ValidMember(m)) return nullptr;
    std::lock_guard lock(mutex);
    auto key = Key(m, stat);
    if (auto it = fields.find(key); it != fields.end()) return it->second;
    m = lunar1710::Translate(m);
    if (!ValidMember(m)) return nullptr;
    auto type = Class(env, m.owner);
    auto id = type ? (stat ? env->GetStaticFieldID(type, m.name, m.descriptor) : env->GetFieldID(type, m.name, m.descriptor)) : nullptr;
    if (!Clean(env)) id = nullptr;
    if (!id) LogTo("JNI access: missing field %s", key.c_str());
    fields.emplace(std::move(key), id); return id;
}
void Release(JNIEnv* env) {
    std::lock_guard lock(mutex);
    for (auto& [name, type] : classes) if (type) env->DeleteGlobalRef(type);
    classes.clear(); methods.clear(); fields.clear();
}
}
