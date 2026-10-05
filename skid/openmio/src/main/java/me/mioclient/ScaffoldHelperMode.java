package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ScaffoldHelperMode.class */
public enum ScaffoldHelperMode implements EnumSettingHelper {
    NONE("None"),
    NORMAL("Normal"),
    SILENT("Silent");

    public final String name;

    ScaffoldHelperMode(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }
}
