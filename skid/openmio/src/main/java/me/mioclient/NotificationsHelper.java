package me.mioclient;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;
import me.mioclient.module.Module;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/NotificationsHelper.class */
public final class NotificationsHelper implements SearchHelper_4, PresetHelper_7 {
    public final Map<Module, String> map = new HashMap();
    public final Map<String, String> map2 = new HashMap();

    public void do393(Module module, String str) {
        this.map.compute(module, (module2, str2) -> {
            return str;
        });
    }

    public void do394(String str, String str2) {
        this.map2.compute(str, (str3, str4) -> {
            return str2;
        });
    }

    public void do395(Module module) {
        this.map.remove(module);
    }

    public void do396(String str) {
        this.map2.remove(str);
    }

    public String getString397(Module module) {
        return this.map.getOrDefault(module, module.getName());
    }

    public String getString398(String str) {
        return this.map2.getOrDefault(str, str);
    }

    @Override // me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        JsonObject jsonObject = new JsonObject();
        JsonObject jsonObject2 = new JsonObject();
        JsonObject jsonObject3 = new JsonObject();
        this.map.forEach((module, str) -> {
            jsonObject2.addProperty(module.getName(), str);
        });
        Map<String, String> map = this.map2;
        Objects.requireNonNull(jsonObject3);
        map.forEach(jsonObject3::addProperty);
        jsonObject.add("module", jsonObject2);
        jsonObject.add("players", jsonObject3);
        return jsonObject;
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        JsonObject asJsonObject = jsonElement.getAsJsonObject();
        if (asJsonObject.has("module")) {
            asJsonObject.getAsJsonObject("module").asMap().forEach((str, jsonElement2) -> {
                BaritoneHelper_3.keyPearlSearchHelper4.getOptional2404(module -> {
                    return module.getName().equalsIgnoreCase(str);
                }).ifPresent(module2 -> {
                    do393(module2, jsonElement2.getAsString());
                });
            });
        }
        if (asJsonObject.has("players")) {
            asJsonObject.getAsJsonObject("players").asMap().forEach((str2, jsonElement3) -> {
                do394(str2, jsonElement3.getAsString());
            });
        }
    }

    @Override // me.mioclient.PresetHelper_7
    public String getConfigName() {
        return "aliases.json";
    }
}
