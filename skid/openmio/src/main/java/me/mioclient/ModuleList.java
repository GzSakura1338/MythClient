package me.mioclient;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import java.awt.Color;
import java.util.Iterator;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.module.Module;
import me.mioclient.module.client.HUD;
import net.minecraft.client.gui.DrawContext;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ModuleList.class */
public abstract class ModuleList extends Module {
    public static final EnumSettingConverter enumSettingConverter = new EnumSettingConverter(ModuleListMode.class);
    public static HUD hud = (HUD) BaritoneHelper_3.baritoneHelper_4.getModule117(HUD.class);
    public ModuleListSearchHelper4 moduleListSearchHelper4;

    public ModuleList(String str, String... strArr) {
        super(str, Category.HUD, strArr);
    }

    public void do364(DrawContext drawContext) {
    }

    public float[] getFloatArray365() {
        throw new java.lang.RuntimeException();
    }

    @Override // me.mioclient.module.Module, me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        JsonObject jsonObject = new JsonObject();
        jsonObject.addProperty("enabled", Boolean.valueOf(isToggled()));
        JsonObject jsonObject2 = new JsonObject();
        for (Setting<?> setting : getRegistry()) {
            if (!setting.is2352()) {
                jsonObject2.add(setting.getConfigName(), setting.toJson());
            }
        }
        jsonObject.add("settings", (JsonElement) jsonObject2);
        JsonObject asJsonObject = jsonObject.getAsJsonObject();
        JsonObject jsonObject3 = new JsonObject();
        jsonObject3.add("anchor", enumSettingConverter.doForward(this.moduleListSearchHelper4.getModuleListMode2818()));
        jsonObject3.addProperty("index", Integer.valueOf(get1296()));
        if (this.moduleListSearchHelper4.getModuleListMode2818() == ModuleListMode.NONE) {
            jsonObject3.addProperty("x", Float.valueOf(this.moduleListSearchHelper4.get2945()));
            jsonObject3.addProperty("y", Float.valueOf(this.moduleListSearchHelper4.get2947()));
        }
        asJsonObject.add("hud", (JsonElement) jsonObject3);
        return asJsonObject;
    }

    @Override // me.mioclient.module.Module, me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        super.fromJson(jsonElement);
        JsonObject asJsonObject = jsonElement.getAsJsonObject();
        if (asJsonObject.has("hud")) {
            JsonObject asJsonObject2 = asJsonObject.getAsJsonObject("hud");
            do2952((ModuleListMode) enumSettingConverter.doBackward(asJsonObject2.get("anchor")));
            if (this.moduleListSearchHelper4.getModuleListMode2818() == ModuleListMode.NONE) {
                this.moduleListSearchHelper4.do2946(asJsonObject2.get("x").getAsFloat(), false);
                this.moduleListSearchHelper4.do2948(asJsonObject2.get("y").getAsFloat(), false);
            }
        }
    }

    @Override // me.mioclient.module.Module
    public boolean isDrawn() {
        return false;
    }

    public boolean is3017() {
        return minecraftClient.currentScreen instanceof HUDSearchHelper4;
    }

    public ModuleListMode getModuleListMode2818() {
        return this.moduleListSearchHelper4.getModuleListMode2818();
    }

    public Color getColor3018(float f) {
        return hud.setting8.getValue().getColor1911(hud, f);
    }

    public void do2952(ModuleListMode moduleListMode) {
        this.moduleListSearchHelper4.do2952(moduleListMode);
        BaritoneHelper_3.getHUDSearchHelper42217().do197(this.moduleListSearchHelper4);
        BaritoneHelper_3.getHUDSearchHelper42217().do195(moduleListMode, this.moduleListSearchHelper4);
    }

    public void do3019(ModuleListSearchHelper4 moduleListSearchHelper4) {
        this.moduleListSearchHelper4 = moduleListSearchHelper4;
    }

    public int get1296() {
        int i = 0;
        Iterator<ModuleListSearchHelper4> it = BaritoneHelper_3.getHUDSearchHelper42217().getSearchHelper4_10200(this.moduleListSearchHelper4.getModuleListMode2818()).getArrayList2819().iterator();
        while (it.hasNext()) {
            if (it.next().getModuleList2958().equals(this)) {
                return i;
            }
            i++;
        }
        return -1;
    }

    public ModuleListSearchHelper4 getModuleListSearchHelper43020() {
        return this.moduleListSearchHelper4;
    }
}
