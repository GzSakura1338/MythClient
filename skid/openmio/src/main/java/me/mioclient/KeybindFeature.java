package me.mioclient;

import java.util.ArrayList;
import java.util.List;
import me.mioclient.api.Keybind;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/KeybindFeature.class */
public abstract class KeybindFeature extends me.mioclient.module.Feature {
    public final List<String> list;
    public final Mode_4 mode_4;
    public final Keybind keybind;

    public KeybindFeature(String str, Mode_4 mode_4, Keybind keybind) {
        super(str);
        this.list = new ArrayList();
        this.mode_4 = mode_4;
        this.keybind = keybind;
    }

    public abstract void run();

    public Mode_4 getMode_42058() {
        return this.mode_4;
    }

    public Keybind getKeybind() {
        return this.keybind;
    }

    public List<String> getList2059() {
        return this.list;
    }

    public void do2060(String str) {
        for (String str2 : str.split(";")) {
            ChatFilterSearchHelper4_2.do2060(str2.trim());
        }
    }
}
