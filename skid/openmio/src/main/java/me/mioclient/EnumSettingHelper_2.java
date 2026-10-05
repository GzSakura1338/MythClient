package me.mioclient;

import com.google.gson.annotations.SerializedName;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/EnumSettingHelper_2.class */
public final class EnumSettingHelper_2 implements EnumSettingHelper {

    @SerializedName("sha")
    public final String string;

    @SerializedName("message")
    public final String name;

    @SerializedName("author")
    public final String string2;

    public EnumSettingHelper_2(String str, String str2, String str3) {
        this.string = str;
        this.name = str2;
        this.string2 = str3;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }

    public String getString2183() {
        return this.string.substring(0, 6);
    }




    @SerializedName("sha")
    public String getString2184() {
        return this.string;
    }

    @SerializedName("message")
    public String getString2185() {
        return this.name;
    }

    @SerializedName("author")
    public String getString2186() {
        return this.string2;
    }
}
