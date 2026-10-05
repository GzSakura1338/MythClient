package me.mioclient;

import com.google.gson.JsonElement;
import com.google.gson.JsonPrimitive;
import me.mioclient.api.Setting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/StringSetting.class */
public final class StringSetting extends Setting<String> {
    public StringSetting(String str, String str2, java.util.function.Predicate<String> predicate) {
        super(str, str2, predicate);
    }

    public StringSetting(String str, String str2) {
        super(str, str2);
    }

    @Override // me.mioclient.api.Setting
    public void do134(String str) {
        do2333(str);
    }

    @Override // me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        return new JsonPrimitive(getValue());
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        do2333(jsonElement.getAsString());
    }
}
