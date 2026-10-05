package me.mioclient;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Map;
import java.util.Optional;
import me.mioclient.module.Module;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/PresetHelperModeHelper_2.class */
public class PresetHelperModeHelper_2 {
    public static final PresetHelperModeHelper_2 presetHelperModeHelper_2 = new PresetHelperModeHelper_2();

    public void do2095(JsonObject jsonObject) {
        for (JsonElement jsonObject2 : jsonObject.asMap().values()) {
            if (jsonObject2 instanceof JsonObject) {
                JsonObject jsonObject3 = (JsonObject) jsonObject2;
                jsonObject3.remove("key");
                if (jsonObject3.has("settings")) {
                    JsonElement jsonElement = jsonObject3.get("settings");
                    if (jsonElement.isJsonObject()) {
                        ArrayList arrayList = new ArrayList();
                        for (Map.Entry entry : jsonElement.getAsJsonObject().asMap().entrySet()) {
                            if (((JsonElement) entry.getValue()).isJsonObject()) {
                                arrayList.add((String) entry.getKey());
                            }
                        }
                        Iterator it = arrayList.iterator();
                        while (it.hasNext()) {
                            jsonElement.getAsJsonObject().remove((String) it.next());
                        }
                    }
                }
            }
        }
    }

    public void do2096(JsonObject jsonObject, java.util.function.Predicate<Module> predicate) {
        for (String str : jsonObject.asMap().keySet()) {
            JsonObject jsonObject2 = (jsonObject.get(str)) instanceof JsonObject ? (JsonObject) (jsonObject.get(str)) : null;
            if (jsonObject2 instanceof JsonObject) {
                JsonObject jsonObject3 = jsonObject2;
                Optional optional2404 = BaritoneHelper_3.keyPearlSearchHelper4.getOptional2404(module -> {
                    return module.getConfigName().equals(str);
                });
                if (!optional2404.isEmpty()) {
                    boolean test = predicate.test((Module) optional2404.get());
                    if (!test) {
                        jsonObject3.remove("enabled");
                    }
                    jsonObject3.remove("key");
                    if (jsonObject3.has("settings")) {
                        JsonElement jsonElement = jsonObject3.get("settings");
                        if (jsonElement.isJsonObject()) {
                            ArrayList arrayList = new ArrayList();
                            for (Map.Entry entry : jsonElement.getAsJsonObject().asMap().entrySet()) {
                                if (test) {
                                    break;
                                } else if (!((JsonElement) entry.getValue()).isJsonObject()) {
                                    arrayList.add((String) entry.getKey());
                                }
                            }
                            Iterator it = arrayList.iterator();
                            while (it.hasNext()) {
                                jsonElement.getAsJsonObject().remove((String) it.next());
                            }
                        }
                    }
                }
            }
        }
    }

    public void do2097(JsonObject jsonObject) {
        for (JsonElement jsonObject2 : jsonObject.asMap().values()) {
            if (jsonObject2 instanceof JsonObject) {
                JsonObject jsonObject3 = (JsonObject) jsonObject2;
                jsonObject3.remove("enabled");
                jsonObject3.remove("settings");
            }
        }
    }
}
