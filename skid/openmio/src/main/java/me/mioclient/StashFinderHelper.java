package me.mioclient;

import com.google.gson.annotations.SerializedName;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/StashFinderHelper.class */
public class StashFinderHelper implements EnumSettingHelper {

    @SerializedName("id")
    public final String name;

    @SerializedName("x")
    public final int num;

    @SerializedName("z")
    public final int num2;

    @SerializedName("dimension")
    public final String string;

    @SerializedName("server")
    public final String string2;

    public StashFinderHelper(String str, int i, int i2, String str2, String str3) {
        this.name = str;
        this.num = i;
        this.num2 = i2;
        this.string = str2;
        this.string2 = str3;
    }

    public String getString514() {
        return new ArgumentTypeHelper().getArgumentTypeHelper2906(this.num2).getArgumentTypeHelper2906(this.num).getString2921("\u0001, \u0001.");
    }

    public double get515() {
        return this.num;
    }

    public double get516() {
        return this.num2;
    }

    public String getString517() {
        return this.string;
    }

    public String getString518() {
        return this.string2;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }
}
