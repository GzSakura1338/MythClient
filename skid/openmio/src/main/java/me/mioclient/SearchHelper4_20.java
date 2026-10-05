package me.mioclient;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import java.util.AbstractMap;
import java.util.Comparator;
import me.mioclient.event.Listen;
import me.mioclient.module.Armor;
import me.mioclient.module.Chat;
import me.mioclient.module.Crypto;
import me.mioclient.module.Direction;
import me.mioclient.module.Effects;
import me.mioclient.module.EntityList;
import me.mioclient.module.Graph;
import me.mioclient.module.Inventory;
import me.mioclient.module.Lag;
import me.mioclient.module.Map;
import me.mioclient.module.Metrics;
import me.mioclient.module.Module;
import me.mioclient.module.Music;
import me.mioclient.module.PlayerModel;
import me.mioclient.module.Position;
import me.mioclient.module.TextRadar;
import me.mioclient.module.Totems;
import me.mioclient.module.Watermark;
import me.mioclient.module.Welcomer;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper4_20.class */
public final class SearchHelper4_20 implements SearchHelper_4, PresetHelper_7 {
    public SearchHelper4_20() {
        baritoneHelper.do1796(this);
        BaritoneHelper_3.keyPearlSearchHelper4.register(new Watermark());
        BaritoneHelper_3.keyPearlSearchHelper4.register(new me.mioclient.module.ModuleList());
        BaritoneHelper_3.keyPearlSearchHelper4.register(new Armor());
        BaritoneHelper_3.keyPearlSearchHelper4.register(new Graph());
        BaritoneHelper_3.keyPearlSearchHelper4.register(new Effects());
        BaritoneHelper_3.keyPearlSearchHelper4.register(new Welcomer());
        BaritoneHelper_3.keyPearlSearchHelper4.register(new Position());
        BaritoneHelper_3.keyPearlSearchHelper4.register(new Direction());
        BaritoneHelper_3.keyPearlSearchHelper4.register(new TextRadar());
        BaritoneHelper_3.keyPearlSearchHelper4.register(new Metrics());
        BaritoneHelper_3.keyPearlSearchHelper4.register(new Totems());
        BaritoneHelper_3.keyPearlSearchHelper4.register(new Lag());
        BaritoneHelper_3.keyPearlSearchHelper4.register(new Inventory());
        BaritoneHelper_3.keyPearlSearchHelper4.register(new Music());
        BaritoneHelper_3.keyPearlSearchHelper4.register(new PlayerModel());
        BaritoneHelper_3.keyPearlSearchHelper4.register(new EntityList());
        BaritoneHelper_3.keyPearlSearchHelper4.register(new Chat());
        BaritoneHelper_3.keyPearlSearchHelper4.register(new Map());
        BaritoneHelper_3.keyPearlSearchHelper4.register(new Crypto());
    }

    @Listen
    public void onEvent(ClientEvent clientEvent) {
        for (Module module : BaritoneHelper_3.keyPearlSearchHelper4.getRegistry()) {
            if (module instanceof ModuleList) {
                ModuleList moduleList = (ModuleList) module;
                moduleList.do2952(moduleList.getModuleListMode2818());
            }
        }
    }

    @Override // me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        JsonObject jsonObject = new JsonObject();
        for (Module module : BaritoneHelper_3.keyPearlSearchHelper4.getRegistry()) {
            if (module instanceof ModuleList) {
                try {
                    jsonObject.add(module.getConfigName(), module.toJson());
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
        return jsonObject;
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        jsonElement.getAsJsonObject().entrySet().stream().sorted(Comparator.comparing(entry -> {
            try {
                return Integer.valueOf(((JsonElement) entry.getValue()).getAsJsonObject().getAsJsonObject("hud").get("index").getAsInt());
            } catch (Throwable th) {
                return 0;
            }
        })).map(entry2 -> {
            return new AbstractMap.SimpleEntry((Module) BaritoneHelper_3.keyPearlSearchHelper4.getOptional2404(module -> {
                return module.getConfigName().equalsIgnoreCase((String) entry2.getKey());
            }).orElse(null), (JsonElement) entry2.getValue());
        }).filter(simpleEntry -> {
            return simpleEntry.getKey() instanceof ModuleList;
        }).forEach(simpleEntry2 -> {
            try {
                ((Module) simpleEntry2.getKey()).fromJson((JsonElement) simpleEntry2.getValue());
            } catch (Exception e) {
                e.printStackTrace();
            }
        });
    }

    @Override // me.mioclient.PresetHelper_7
    public String getConfigName() {
        return "hud.json";
    }
}
