package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ChestStealerMode.class */
public enum ChestStealerMode implements EnumSettingHelper {
    STEAL("Steal"),
    FILL("Fill"),
    DROP("Drop"),
    REFILL("Refill"),
    REKIT("Rekit");

    public final String name;

    ChestStealerMode(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }
}
