#include "minecraft/modules/misc/imefix/ChatInput.h"
#include "minecraft/compat/lunar12111/Mapping.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/java/api/jni_lookup.h"
#include <algorithm>

namespace nova::module::imefix {
namespace {
constexpr auto Minecraft = "net.minecraft.client.Minecraft";
constexpr auto Chat = "net.minecraft.client.gui.screens.ChatScreen";
constexpr auto Edit = "net.minecraft.client.gui.components.EditBox";

jclass Load(JNIEnv* env, jobject loader, const char* name) {
    auto type = LoadClassInLoader(env, loader, compat::IsLunar189() ? name : compat::lunar12111::ClassName(name));
    if (env->ExceptionCheck()) env->ExceptionClear();
    return type;
}
jmethodID Method(JNIEnv* env, jclass type, const char* owner, const char* name,
                 const char* signature, bool isStatic = false) {
    const auto m = compat::IsLunar189() ? compat::lunar12111::Member{owner,name,signature}
                                      : compat::lunar12111::Translate({owner, name, signature});
    auto result = isStatic ? env->GetStaticMethodID(type, m.name, m.descriptor)
                          : env->GetMethodID(type, m.name, m.descriptor);
    if (env->ExceptionCheck()) env->ExceptionClear();
    return result;
}
jfieldID Field(JNIEnv* env, jclass type, const char* owner, const char* name, const char* signature) {
    const auto m = compat::IsLunar189() ? compat::lunar12111::Member{owner,name,signature}
                                      : compat::lunar12111::Translate({owner, name, signature});
    auto result = env->GetFieldID(type, m.name, m.descriptor);
    if (env->ExceptionCheck()) env->ExceptionClear();
    return result;
}
}

bool ChatInput::Resolve(JNIEnv* env) {
    if (minecraft_) return true;
    auto loader = GetMinecraftClassLoader(env);
    if (!loader) return false;
    const bool legacy=compat::IsLunar189();
    const auto chatName=legacy?"net.minecraft.client.gui.GuiChat":Chat;
    const auto editName=legacy?"net.minecraft.client.gui.GuiTextField":Edit;
    auto mc = Load(env, loader, Minecraft);
    auto chat = Load(env, loader, chatName);
    auto edit = Load(env, loader, editName);
    env->DeleteGlobalRef(loader);
    if (!mc || !chat || !edit) return false;
    getInstance_ = Method(env, mc, Minecraft, legacy?"getMinecraft":"getInstance", "()Lnet/minecraft/client/Minecraft;", true);
    sameThread_ = Method(env, mc, legacy?Minecraft:"net.minecraft.util.thread.BlockableEventLoop",
                         legacy?"isCallingFromMinecraftThread":"isSameThread", "()Z");
    screenField_ = Field(env, mc, Minecraft, legacy?"currentScreen":"screen",
                         legacy?"Lnet/minecraft/client/gui/GuiScreen;":"Lnet/minecraft/client/gui/screens/Screen;");
    worldField_ = Field(env, mc, Minecraft, legacy?"theWorld":"level",
                        legacy?"Lnet/minecraft/client/multiplayer/WorldClient;":"Lnet/minecraft/client/multiplayer/ClientLevel;");
    inputField_ = Field(env, chat, chatName, legacy?"inputField":"input",
                        legacy?"Lnet/minecraft/client/gui/GuiTextField;":"Lnet/minecraft/client/gui/components/EditBox;");
    canConsume_ = Method(env, edit, editName, legacy?"isFocused":"canConsumeInput", "()Z");
    if(legacy)enabledField_=Field(env,edit,editName,"isEnabled","Z");
    insert_ = Method(env, edit, editName, legacy?"writeText":"insertText", "(Ljava/lang/String;)V");
    getValue_ = Method(env, edit, editName, legacy?"getText":"getValue", "()Ljava/lang/String;");
    if (!getInstance_ || !sameThread_ || !screenField_ || !worldField_ ||
        !inputField_ || !canConsume_ || !insert_ || !getValue_ || (legacy&&!enabledField_)) return false;
    minecraft_ = static_cast<jclass>(env->NewGlobalRef(mc));
    chat_ = static_cast<jclass>(env->NewGlobalRef(chat));
    edit_ = static_cast<jclass>(env->NewGlobalRef(edit));
    if (!minecraft_ || !chat_ || !edit_ || env->ExceptionCheck()) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        Release(env);
        return false;
    }
    LogTo("IME Fix: Lunar %s Unicode chat adapter resolved",legacy?"1.8.9":"1.21.11");
    return true;
}

bool ChatInput::OnClientThread(JNIEnv* env) {
    if (!minecraft_) return false;
    auto mc = env->CallStaticObjectMethod(minecraft_, getInstance_);
    if (env->ExceptionCheck() || !mc) return false;
    return env->CallBooleanMethod(mc, sameThread_) == JNI_TRUE && !env->ExceptionCheck();
}

bool ChatInput::RefreshTarget(JNIEnv* env, bool& changed) {
    changed = false;
    auto mc = env->CallStaticObjectMethod(minecraft_, getInstance_);
    if (env->ExceptionCheck() || !mc) return false;
    auto screen = env->GetObjectField(mc, screenField_);
    if (env->ExceptionCheck()) return false;
    auto world = env->GetObjectField(mc, worldField_);
    if (env->ExceptionCheck()) return false;
    if (!world || !screen || !env->IsInstanceOf(screen, chat_)) {
        changed = screen_ != nullptr;
        ClearTarget(env);
        return false;
    }
    auto input = env->GetObjectField(screen, inputField_);
    if (env->ExceptionCheck()) return false;
    if (!input || env->CallBooleanMethod(input, canConsume_) != JNI_TRUE || env->ExceptionCheck() ||
        (enabledField_ && env->GetBooleanField(input,enabledField_)!=JNI_TRUE)) {
        changed = screen_ != nullptr;
        ClearTarget(env);
        return false;
    }
    if (!env->IsSameObject(screen, screen_) || !env->IsSameObject(input, input_) ||
        !env->IsSameObject(world, world_)) {
        changed = true;
        ClearTarget(env);
        screen_ = env->NewGlobalRef(screen);
        input_ = env->NewGlobalRef(input);
        world_ = env->NewGlobalRef(world);
        if (!screen_ || !input_ || !world_ || env->ExceptionCheck()) {
            ClearTarget(env);
            return false;
        }
    }
    return true;
}

bool ChatInput::Insert(JNIEnv* env, std::u16string_view text) {
    if (!input_) return false;
    static_assert(sizeof(char16_t) == sizeof(jchar));
    const auto value = env->NewString(reinterpret_cast<const jchar*>(text.data()), static_cast<jsize>(text.size()));
    if (!value || env->ExceptionCheck()) return false;
    env->CallVoidMethod(input_, insert_, value);
    env->DeleteLocalRef(value);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        LogTo("IME Fix: chat insert/responder raised; commit consumed to prevent duplicate text");
        return true;
    }
    if (reports_++ < 8) {
        // Metadata only: no chat content, clipboard data or user identifiers.
        auto result = static_cast<jstring>(env->CallObjectMethod(input_, getValue_));
        const auto length = !env->ExceptionCheck() && result ? env->GetStringLength(result) : -1;
        const bool nonAscii = std::any_of(text.begin(), text.end(), [](char16_t c) { return c > 127; });
        LogTo("IME Fix: committed utf16=%zu nonAscii=%d resultingLength=%d", text.size(), nonAscii, length);
    }
    return true;
}

void ChatInput::ClearTarget(JNIEnv* env) {
    if (screen_) env->DeleteGlobalRef(screen_);
    if (input_) env->DeleteGlobalRef(input_);
    if (world_) env->DeleteGlobalRef(world_);
    screen_ = input_ = world_ = nullptr;
}

void ChatInput::Release(JNIEnv* env) {
    ClearTarget(env);
    if (minecraft_) env->DeleteGlobalRef(minecraft_);
    if (chat_) env->DeleteGlobalRef(chat_);
    if (edit_) env->DeleteGlobalRef(edit_);
    *this = {};
}

}
