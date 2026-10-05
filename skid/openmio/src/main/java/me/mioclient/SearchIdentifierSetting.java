package me.mioclient;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import me.mioclient.api.Setting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchIdentifierSetting.class */
public final class SearchIdentifierSetting extends Setting<SearchIdentifier> {
    public SearchIdentifierSetting(String str, SearchIdentifier searchIdentifier) {
        super(str, searchIdentifier);
    }

    public SearchIdentifierSetting(String str, SearchIdentifier searchIdentifier, java.util.function.Predicate<SearchIdentifier> predicate) {
        super(str, searchIdentifier, predicate);
    }

    @Override // me.mioclient.api.Setting
    public void do134(String str) {
        String[] split = str.split(":");
        if (split.length == 1) {
            do2333(new SearchIdentifier(str));
        } else if (split.length == 2) {
            do2333(new SearchIdentifier(split[0], split[1]));
        }
    }

    @Override // me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        JsonObject jsonObject = new JsonObject();
        jsonObject.addProperty("name", getValue().getName());
        jsonObject.addProperty("category", getValue().getString1610());
        return jsonObject;
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        JsonObject asJsonObject = jsonElement.getAsJsonObject();
        if (PresetHelperModeHelper.is435(asJsonObject, "name", "category")) {
            do2333(new SearchIdentifier(asJsonObject.get("category").getAsString(), asJsonObject.get("name").getAsString()));
        }
    }
}
