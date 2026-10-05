package me.mioclient;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import java.awt.Color;
import org.jetbrains.annotations.NotNull;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Converter.class */
public class Converter extends com.google.common.base.Converter<Color, JsonElement> {
    @NotNull
    /* renamed from: getJsonElement2090, reason: merged with bridge method [inline-methods] */
    public JsonElement doForward(Color color) {
        JsonArray jsonArray = new JsonArray();
        jsonArray.add(Integer.valueOf(color.getRed()));
        jsonArray.add(Integer.valueOf(color.getGreen()));
        jsonArray.add(Integer.valueOf(color.getBlue()));
        jsonArray.add(Integer.valueOf(color.getAlpha()));
        return jsonArray;
    }

    @NotNull
    /* renamed from: getColor2091, reason: merged with bridge method [inline-methods] */
    public Color doBackward(JsonElement jsonElement) {
        if (!jsonElement.isJsonArray() || jsonElement.getAsJsonArray().size() < 4) {
            throw new IllegalArgumentException();
        }
        JsonArray asJsonArray = jsonElement.getAsJsonArray();
        return new Color(asJsonArray.get(0).getAsInt(), asJsonArray.get(1).getAsInt(), asJsonArray.get(2).getAsInt(), asJsonArray.get(3).getAsInt());
    }
}
