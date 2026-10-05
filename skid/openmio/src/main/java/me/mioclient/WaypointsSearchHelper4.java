package me.mioclient;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Iterator;
import java.util.List;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/WaypointsSearchHelper4.class */
public final class WaypointsSearchHelper4 extends ModuleListHelper<WaypointsEnumSettingHelper, List<WaypointsEnumSettingHelper>> implements SearchHelper_4, PresetHelper_7 {
    public WaypointsSearchHelper4() {
        super(new ArrayList());
        baritoneHelper.do1796(this);
    }

    public List<WaypointsEnumSettingHelper> getVisible() {
        if (is1469()) {
            return Collections.emptyList();
        }
        return ((List<WaypointsEnumSettingHelper>) this.registry).stream().filter(waypointsEnumSettingHelper -> {
            return waypointsEnumSettingHelper.getString518().equalsIgnoreCase(minecraftClient.player.networkHandler.getServerInfo() == null ? "singleplayer" : minecraftClient.player.networkHandler.getServerInfo().address);
        }).sorted(Comparator.comparing(waypointsEnumSettingHelper2 -> {
            return Double.valueOf(minecraftClient.player.squaredDistanceTo(waypointsEnumSettingHelper2.get515(), waypointsEnumSettingHelper2.get692(), waypointsEnumSettingHelper2.get516()));
        })).toList();
    }

    @Override // me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        JsonArray jsonArray = new JsonArray();
        JsonObject jsonObject = new JsonObject();
        Iterator it = ((List) this.registry).iterator();
        while (it.hasNext()) {
            jsonArray.add(gson.toJsonTree((WaypointsEnumSettingHelper) it.next()));
        }
        jsonObject.add("waypoints", (JsonElement) jsonArray);
        return jsonObject;
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        if (jsonElement.isJsonObject() && jsonElement.getAsJsonObject().has("waypoints")) {
            Iterator it = jsonElement.getAsJsonObject().getAsJsonArray("waypoints").iterator();
            while (it.hasNext()) {
                JsonElement jsonElement2 = (JsonElement) it.next();
                try {
                    ((List) this.registry).add((WaypointsEnumSettingHelper) gson.fromJson(jsonElement2, WaypointsEnumSettingHelper.class));
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
    }

    @Override // me.mioclient.PresetHelper_7
    public String getConfigName() {
        return "waypoints.json";
    }

    @Override // me.mioclient.Helper_9
    /* renamed from: is2267, reason: merged with bridge method [inline-methods] */
    public boolean register(WaypointsEnumSettingHelper waypointsEnumSettingHelper) {
        for (WaypointsEnumSettingHelper waypointsEnumSettingHelper2 : getRegistry()) {
            if (waypointsEnumSettingHelper2.getName().equalsIgnoreCase(waypointsEnumSettingHelper.getName())) {
                if (waypointsEnumSettingHelper2.getString518().equalsIgnoreCase(waypointsEnumSettingHelper.getString518())) {
                    return false;
                }
            }
        }
        return getRegistry().add(waypointsEnumSettingHelper);
    }

    @Override // me.mioclient.Helper_9
    /* renamed from: is2268, reason: merged with bridge method [inline-methods] */
    public boolean unregister(WaypointsEnumSettingHelper waypointsEnumSettingHelper) {
        return getRegistry().removeIf(waypointsEnumSettingHelper2 -> {
            if (waypointsEnumSettingHelper2.getName().equalsIgnoreCase(waypointsEnumSettingHelper.getName())) {
                if (waypointsEnumSettingHelper2.getString518().equalsIgnoreCase(waypointsEnumSettingHelper.getName())) {
                    return true;
                }
            }
            return false;
        });
    }
}
