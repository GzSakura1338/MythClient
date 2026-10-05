package me.mioclient;

import com.google.gson.JsonElement;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/PresetHelper_7.class */
public interface PresetHelper_7 {
    JsonElement toJson();

    void fromJson(JsonElement jsonElement);

    default String getConfigName() {
        return "";
    }
}
