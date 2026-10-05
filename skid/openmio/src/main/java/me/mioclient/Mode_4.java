package me.mioclient;

import java.util.function.BiFunction;
import me.mioclient.api.Keybind;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Mode_4.class */
public enum Mode_4 implements EnumSettingHelper {
    SIMPLE("simple", KeybindFeature_2::new),
    QUEUE("queue", KeybindFeature_4::new),
    DOUBLE_TAP("double_tap", StopwatchKeybindFeature::new),
    HOLD("hold", KeybindFeature_3::new);

    public final String name;
    public final BiFunction<String, Keybind, KeybindFeature> biFunction;

    Mode_4(String str, BiFunction<String, Keybind, KeybindFeature> biFunction) {
        this.name = str;
        this.biFunction = biFunction;
    }

    public static Mode_4 getMode_4831(String str) {
        for (Mode_4 mode_4 : values()) {
            if (mode_4.getName().equalsIgnoreCase(str)) {
                return mode_4;
            }
        }
        return null;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }

    public KeybindFeature getKeybindFeature832(String str, Keybind keybind) {
        return this.biFunction.apply(str, keybind);
    }
}
