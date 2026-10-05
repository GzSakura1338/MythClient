package me.mioclient;

import com.google.gson.JsonElement;
import com.google.gson.JsonParser;
import java.nio.file.Path;
import java.util.Iterator;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/PresetHelperSearchHelper4.class */
public final class PresetHelperSearchHelper4 implements SearchHelper_4 {
    public static void run() {
        Path resolve = PresetHelper.path.resolve("friends.json");
        Path resolve2 = PresetHelper.path.resolve("socials.json");
        if (!resolve.toFile().exists() || resolve2.toFile().exists()) {
            return;
        }
        Iterator it = JsonParser.parseString(PresetHelper_4.getString1570(resolve)).getAsJsonObject().getAsJsonArray("friends").iterator();
        while (it.hasNext()) {
            JsonElement jsonElement = (JsonElement) it.next();
            BaritoneHelper_3.searchHelper4_14.do523(jsonElement.getAsString());
        }
        PresetHelper_4.do1567(resolve2, gson.toJson(BaritoneHelper_3.searchHelper4_14.toJson()));
        resolve.toFile().delete();
    }
}
