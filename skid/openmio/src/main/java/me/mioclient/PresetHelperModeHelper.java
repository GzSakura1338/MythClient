package me.mioclient;

import com.google.gson.JsonObject;
import java.util.Map;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/PresetHelperModeHelper.class */
public class PresetHelperModeHelper {
    public static boolean is435(JsonObject jsonObject, String... strArr) {
        for (String str : strArr) {
            if (!jsonObject.has(str)) {
                return false;
            }
        }
        return true;
    }

    public static boolean is436(JsonObject jsonObject) {
        for (Map.Entry entry : jsonObject.entrySet()) {
            if (!(entry.getValue() instanceof JsonObject) || !is436((JsonObject) entry.getValue())) {
                return false;
            }
        }
        return true;
    }
}
