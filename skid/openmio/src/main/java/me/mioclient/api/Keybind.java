package me.mioclient.api;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import me.mioclient.EntityControlSearchHelper4;
import me.mioclient.EnumSettingHelper;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/api/Keybind.class */
public final class Keybind {
    public final int num;
    public final KeybindMode keybindMode;
    public final boolean flag;
    public static final Keybind keybind = new Keybind(-1, KeybindMode.TOGGLE, false);

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/api/Keybind$KeybindMode.class */
    public enum KeybindMode implements EnumSettingHelper {
        TOGGLE("Toggle", "toggle"),
        HOLD("Hold", "hold"),
        HOLD_REVERSE("ReverseHold", "hold_reverse");

        public final String name;
        public final String string;

        KeybindMode(String str, String str2) {
            this.name = str;
            this.string = str2;
        }

        public String getString2552() {
            return this.string;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }

        public static KeybindMode getKeybindMode2553(String str) {
            String lowerCase = str.trim().toLowerCase();
            int z = -1;
            switch (lowerCase.hashCode()) {
                case 3208383:
                    if (lowerCase.equals("hold")) {
                        z = 0;
                        break;
                    }
                    break;
                case 1999224674:
                    if (lowerCase.equals("hold_reverse")) {
                        z = 1;
                        break;
                    }
                    break;
            }
            switch (z) {
                case 0:
                    return HOLD;
                case 1:
                    return HOLD_REVERSE;
                default:
                    return TOGGLE;
            }
        }
    }

    public Keybind(int i, KeybindMode keybindMode, boolean z) {
        this.num = i;
        this.keybindMode = keybindMode;
        this.flag = z;
    }

    public Keybind getKeybind1941(int i) {
        return new Keybind(i, this.keybindMode, this.flag);
    }

    public Keybind getKeybind1942(KeybindMode keybindMode) {
        return new Keybind(this.num, keybindMode, this.flag);
    }

    public Keybind getKeybind1943(boolean z) {
        return new Keybind(this.num, this.keybindMode, z);
    }

    public boolean is1944() {
        return this.num < 0;
    }

    public String getString773() {
        return EntityControlSearchHelper4.getString2602(this);
    }




    public int get1945() {
        return this.num;
    }

    public KeybindMode getKeybindMode1946() {
        return this.keybindMode;
    }

    public boolean is1947() {
        return this.flag;
    }
}
