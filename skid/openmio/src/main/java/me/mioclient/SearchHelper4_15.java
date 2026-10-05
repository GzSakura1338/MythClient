package me.mioclient;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import java.util.Iterator;
import java.util.List;
import me.mioclient.module.misc.StashFinder;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper4_15.class */
public final class SearchHelper4_15 extends StashFinderModuleListHelper<StashFinderHelper> implements SearchHelper_4, PresetHelper_7 {
    public static StashFinder stashFinder = (StashFinder) BaritoneHelper_3.baritoneHelper_4.getModule117(StashFinder.class);

    public SearchHelper4_15() {
        baritoneHelper.do1796(this);
    }

    @Override // me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        if (!stashFinder.log.getValue().booleanValue()) {
            return new JsonObject();
        }
        JsonArray jsonArray = new JsonArray();
        JsonObject jsonObject = new JsonObject();
        Iterator it = BaritoneHelper_3.searchHelper4_15.getRegistry().iterator();
        while (it.hasNext()) {
            jsonArray.add(gson.toJsonTree((StashFinderHelper) it.next()));
        }
        jsonObject.add("stashes", (JsonElement) jsonArray);
        return jsonObject;
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        if (jsonElement.isJsonObject() && jsonElement.getAsJsonObject().has("stashes")) {
            Iterator it = jsonElement.getAsJsonObject().getAsJsonArray("stashes").iterator();
            while (it.hasNext()) {
                JsonElement jsonElement2 = (JsonElement) it.next();
                try {
                    ((List) this.registry).add((StashFinderHelper) gson.fromJson(jsonElement2, StashFinderHelper.class));
                } catch (Exception e) {
                }
            }
        }
    }

    @Override // me.mioclient.PresetHelper_7
    public String getConfigName() {
        return "stashfinder.json";
    }
}
