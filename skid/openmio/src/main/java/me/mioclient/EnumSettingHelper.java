package me.mioclient;

import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/EnumSettingHelper.class */
public interface EnumSettingHelper {
    String getName();

    default EnumSettingHelper getEnumSettingHelper1878(String str) {
        return () -> {
            return str;
        };
    }

    default Text getText1879() {
        return Text.literal(getName());
    }

    static boolean is1880(Object obj) {
        return obj instanceof EnumSettingHelper;
    }
}
