package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/WarpHelperMode.class */
public enum WarpHelperMode implements EnumSettingHelper {
    HOLD("Hold"),
    INSTANT("Instant"),
    NONE("None");

    public final String name;

    WarpHelperMode(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }
}
