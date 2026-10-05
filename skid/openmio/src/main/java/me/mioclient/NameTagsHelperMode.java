package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/NameTagsHelperMode.class */
public enum NameTagsHelperMode implements EnumSettingHelper {
    FRIEND("friend", "friends"),
    ENEMY("enemy", "enemies");

    public final String name;
    public final String string;

    NameTagsHelperMode(String str, String str2) {
        this.name = str;
        this.string = str2;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }

    public String getString1987() {
        return this.string;
    }
}
