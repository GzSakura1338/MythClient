package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ChestStealerMode_2.class */
public enum ChestStealerMode_2 implements EnumSettingHelper {
    NAME("Name"),
    ITEM("Item");

    public final String name;

    ChestStealerMode_2(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }
}
