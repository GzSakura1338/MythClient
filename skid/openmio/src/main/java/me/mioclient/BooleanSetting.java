package me.mioclient;

import com.google.gson.JsonElement;
import com.google.gson.JsonPrimitive;
import me.mioclient.api.Setting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/BooleanSetting.class */
public final class BooleanSetting extends Setting<Boolean> {
    public BooleanSetting(String str, Boolean bool, java.util.function.Predicate<Boolean> predicate) {
        super(str, bool, predicate);
    }

    public BooleanSetting(String str, Boolean bool) {
        super(str, bool);
    }

    @Override // me.mioclient.api.Setting
    public void do134(String str) {
        if ("toggle".equalsIgnoreCase(str)) {
            do2333(Boolean.valueOf(!getValue().booleanValue()));
            return;
        }
        if (str.equals("0") || str.equalsIgnoreCase("false")) {
            do2333(false);
        } else if (str.equals("1") || str.equalsIgnoreCase("true")) {
            do2333(true);
        }
    }

    @Override // me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        return new JsonPrimitive(getValue());
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        do2333(Boolean.valueOf(jsonElement.getAsBoolean()));
    }
}
