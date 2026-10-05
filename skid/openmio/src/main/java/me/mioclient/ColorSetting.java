package me.mioclient;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import java.awt.Color;
import me.mioclient.api.Setting;
import me.mioclient.module.client.Colors;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ColorSetting.class */
public final class ColorSetting extends Setting<Color> {
    public static Colors colors = (Colors) BaritoneHelper_3.baritoneHelper_4.getModule117(Colors.class);
    public final Converter converter;
    public boolean flag;
    public boolean flag2;

    public ColorSetting(String str, Color color, java.util.function.Predicate<Color> predicate) {
        super(str, color, predicate);
        this.converter = new Converter();
    }

    public ColorSetting(String str, Color color) {
        super(str, color);
        this.converter = new Converter();
    }

    @Override // me.mioclient.api.Setting, me.mioclient.Helper_10
    /* renamed from: getColor2856, reason: merged with bridge method [inline-methods] */
    public Color getValue() {
        Color color = (Color) super.getValue();
        return this.flag2 ? MixinMessageIndicatorHelper_2.getColor816(colors.themeColor.getValue(), color.getAlpha()) : this.flag ? getColor2857(color, 0) : color;
    }

    public Color getColor2857(Color color, int i) {
        float[] RGBtoHSB = Color.RGBtoHSB(color.getRed(), color.getGreen(), color.getBlue(), (float[]) null);
        return MixinMessageIndicatorHelper_2.getColor813(FreecamHelper.num3, i, RGBtoHSB[1], RGBtoHSB[2], color.getAlpha());
    }

    @Override // me.mioclient.api.Setting
    public void do134(String str) {
        try {
            do2333(MixinMessageIndicatorHelper_2.getColor828(str));
        } catch (Throwable th) {
        }
    }

    @Override // me.mioclient.api.Setting
    /* renamed from: do2858, reason: merged with bridge method [inline-methods] */
    public void do2333(Color color) {
        if (this.flag3) {
            super.do2333(new Color(color.hashCode(), false));
        } else {
            super.do2333(color);
        }
    }

    @Override // me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        JsonObject jsonObject = new JsonObject();
        jsonObject.add("rgba", this.converter.doForward(getValue()));
        jsonObject.addProperty("rainbow", Boolean.valueOf(is2862()));
        jsonObject.addProperty("sync", Boolean.valueOf(is2859()));
        return jsonObject;
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        if (jsonElement.isJsonArray()) {
            do2333(this.converter.doBackward(jsonElement));
            return;
        }
        JsonObject asJsonObject = jsonElement.getAsJsonObject();
        do2333(this.converter.doBackward(asJsonObject.get("rgba")));
        do2863(asJsonObject.get("rainbow").getAsBoolean());
        do2860(asJsonObject.get("sync").getAsBoolean());
    }

    public boolean is2859() {
        if (colors == null || this != colors.themeColor) {
            return this.flag2;
        }
        return false;
    }

    public void do2860(boolean z) {
        if (colors == null || this != colors.themeColor) {
            this.flag2 = z;
        }
    }

    public boolean is2861() {
        return this == colors.themeColor;
    }

    public boolean is2862() {
        return this.flag;
    }

    public void do2863(boolean z) {
        this.flag = z;
    }
}
