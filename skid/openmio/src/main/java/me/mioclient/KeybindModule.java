package me.mioclient;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import me.mioclient.api.Category;
import me.mioclient.api.Keybind;
import me.mioclient.module.Module;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/KeybindModule.class */
public abstract class KeybindModule extends Module {
    public KeybindModule(String str, String str2, Category category, String... strArr) {
        super(str, str2, category, strArr);
    }

    public KeybindModule(String str, Category category, String... strArr) {
        this(str, "", category, strArr);
    }

    @Override // me.mioclient.module.Module
    public boolean isDrawn() {
        return false;
    }

    @Override // me.mioclient.module.Module, me.mioclient.HUDHelper
    public boolean isToggled() {
        return true;
    }

    @Override // me.mioclient.module.Module, me.mioclient.HUDHelper
    public void disable() {
    }

    @Override // me.mioclient.module.Module
    public Keybind getKeybind() {
        return Keybind.keybind;
    }

    @Override // me.mioclient.module.Module, me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        JsonObject asJsonObject = super.toJson().getAsJsonObject();
        asJsonObject.remove("enabled");
        asJsonObject.remove("key");
        asJsonObject.remove("state");
        return asJsonObject;
    }
}
