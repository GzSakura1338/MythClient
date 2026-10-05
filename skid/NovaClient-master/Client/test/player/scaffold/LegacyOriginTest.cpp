// Exercise the production JNI adapter with 1.7 Object.toString() output and
// registry names separately. A symbol-only mapping audit cannot detect this.
#include <cassert>
#include <iostream>
#include <string>
#include <string_view>

using jobject = const char*;
using jstring = const char*;
using jclass = const char*;
using jmethodID = const char*;
constexpr int JNI_OK = 0;
bool legacy17 = true, registryAvailable = true, nameAvailable = true;
int registryReads = 0, descriptionReads = 0;
std::string registryName = "minecraft:stone";
std::string description = "net.minecraft.block.Block@17abcd";
struct JNIEnv {
    int PushLocalFrame(int) { return 0; }
    void PopLocalFrame(jobject) {}
    bool ExceptionCheck() { return false; }
    void ExceptionClear() {}
    jclass GetObjectClass(jobject) { return "Block"; }
    jmethodID GetMethodID(jclass, const char* name, const char*) { return name; }
    jobject GetStaticObjectField(jclass, const char*) {
        ++registryReads;
        return registryAvailable ? "registry" : nullptr;
    }
    jobject CallObjectMethod(jobject, jmethodID method, jobject = nullptr) {
        if (std::string_view(method) == "getNameForObject")
            return nameAvailable ? registryName.c_str() : nullptr;
        ++descriptionReads;
        return description.c_str();
    }
    const char* GetStringUTFChars(jstring value, void*) { return value; }
    void ReleaseStringUTFChars(jstring, const char*) {}
} jni;
struct { jclass Block = "Block"; const char* BlockRegistry = "blockRegistry";
    jmethodID RegistryName = "getNameForObject"; } h;
bool IsLunar1710() { return legacy17; }
bool Good(JNIEnv*) { return true; }
bool StringText(JNIEnv*, jstring value, std::string& out) {
    if (!value) return false;
    out = value;
    return true;
}
namespace java {
// @EXCLUSION_METHODS@
}
bool ExcludedOrigin() {
    auto* env = &jni;
    struct { jobject block = "stone"; } read;
    // @ORIGIN_ADAPTER@
}
int main() {
    // Same solid origin, same block; only the version-specific name API differs.
    assert(java::ScaffoldExcludedObject(&jni, "stone")); // Old path rejected 1.7 stone.
    descriptionReads = 0;
    assert(!ExcludedOrigin());
    assert(registryReads == 1 && descriptionReads == 0);
    for (const char* id : {"minecraft:stone", "minecraft:planks", "minecraft:wool", "minecraft:dirt"}) {
        registryName = id;
        assert(!ExcludedOrigin());
    }
    for (const char* id : {"minecraft:chest", "minecraft:trapped_chest", "minecraft:anvil",
             "minecraft:crafting_table", "minecraft:furnace", "minecraft:lit_furnace",
             "minecraft:hopper", "minecraft:dropper", "minecraft:dispenser",
             "minecraft:noteblock", "minecraft:note_block", "minecraft:jukebox"}) {
        registryName = id;
        assert(ExcludedOrigin());
    }
    registryAvailable = false;
    assert(ExcludedOrigin());
    registryAvailable = true;
    nameAvailable = false;
    assert(ExcludedOrigin());
    nameAvailable = true;
    registryName.clear();
    assert(ExcludedOrigin());
    legacy17 = false;
    const int previousReads = registryReads;
    description = "Block{minecraft:stone}";
    assert(!ExcludedOrigin());
    description = "Block{minecraft:chest}";
    assert(ExcludedOrigin());
    description = "Block{minecraft:wool}[color=red]";
    assert(!ExcludedOrigin());
    description = "invalid description";
    assert(ExcludedOrigin());
    assert(registryReads == previousReads);
    std::cout << "PASS: 1.7 registry origins, all excluded blocks, lookup failures, unchanged 1.8+ descriptions\n";
}
