package me.mioclient;

import com.google.gson.JsonObject;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import java.nio.file.Path;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/PresetSearchHelper4.class */
public final class PresetSearchHelper4 implements SearchHelper_4, EnumSettingHelper {
    public final String name;
    public final JsonObject jsonObject;

    public PresetSearchHelper4(String str, JsonObject jsonObject) {
        this.name = str;
        this.jsonObject = jsonObject;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }

    public void do2740(Path path) {
        PresetHelper_4.do1567(path.resolve(new ArgumentTypeHelper().getArgumentTypeHelper2919(this.name).getString2921("\u0001.json")), gson.toJson(getJsonObject2741()));
    }




    public String getString333() {
        return this.name;
    }

    public JsonObject getJsonObject2741() {
        return this.jsonObject;
    }
}
