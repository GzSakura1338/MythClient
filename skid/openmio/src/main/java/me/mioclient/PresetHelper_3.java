package me.mioclient;

import com.google.gson.JsonElement;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/PresetHelper_3.class */
public final class PresetHelper_3 {
    public JsonElement jsonElement;

    public void do41() {
        this.jsonElement = PresetHelperMode.ALL.toJson();
    }

    public JsonElement getJsonElement1434() {
        JsonElement jsonElement = this.jsonElement;
        this.jsonElement = null;
        return jsonElement;
    }
}
