package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/StashFinderMode.class */
public enum StashFinderMode implements EnumSettingHelper {
    OVERWORLD("overworld", "Overworld"),
    THE_NETHER("the_nether", "Nether"),
    THE_END("the_end", "End");

    public final String name;
    public final String string;

    StashFinderMode(String str, String str2) {
        this.name = str;
        this.string = str2;
    }

    public boolean is2172() {
        return this == OVERWORLD;
    }

    public boolean is2173() {
        return this == THE_NETHER;
    }

    public boolean is2174() {
        return this == THE_END;
    }

    public String getString2175() {
        return this.string;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }
}
