package me.mioclient;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import java.util.Iterator;
import java.util.List;
import me.mioclient.MatrixStackEvent;
import me.mioclient.api.Keybind;
import me.mioclient.event.KeyEvent;
import me.mioclient.event.Listen;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper4_12.class */
public final class SearchHelper4_12 extends StashFinderModuleListHelper<KeybindFeature> implements SearchHelper_4, PresetHelper_7 {
    public SearchHelper4_12() {
        baritoneHelper.do1796(this);
    }

    @Listen
    public void onKey(KeyEvent keyEvent) {
        if (minecraftClient.currentScreen != null || is1469()) {
            return;
        }
        for (KeybindFeature keybindFeature : (List<KeybindFeature>) this.registry) {
            if (keybindFeature.getKeybind().is1947() == keyEvent.is2588() && keybindFeature.getKeybind().get1945() == keyEvent.get2587()) {
                keybindFeature.run();
                baritoneHelper.getObject1794(new ClientEvent_2(keybindFeature));
            }
        }
    }

    @Listen
    public void onEvent(MatrixStackEvent.Inner_3 inner_3) {
        if (minecraftClient.currentScreen != null || is1469()) {
            return;
        }
        for (KeybindFeature keybindFeature : (List<KeybindFeature>) this.registry) {
            if (keybindFeature instanceof KeybindFeature_3) {
                KeybindFeature_3 keybindFeature_3 = (KeybindFeature_3) keybindFeature;
                keybindFeature_3.do1072(EntityControlSearchHelper4.is2604(keybindFeature_3.getKeybind()));
            }
        }
    }

    @Override // me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        JsonArray jsonArray = new JsonArray();
        JsonObject jsonObject = new JsonObject();
        for (KeybindFeature keybindFeature : (List<KeybindFeature>) this.registry) {
            JsonElement jsonObject2 = new JsonObject();
            ((JsonObject) jsonObject2).addProperty("name", keybindFeature.getName());
            ((JsonObject) jsonObject2).addProperty("type", keybindFeature.getMode_42058().getName());
            ((JsonObject) jsonObject2).addProperty("key", Integer.valueOf(keybindFeature.getKeybind().get1945()));
            ((JsonObject) jsonObject2).addProperty("mouse", Boolean.valueOf(keybindFeature.getKeybind().is1947()));
            JsonArray jsonArray2 = new JsonArray();
            Iterator<String> it = keybindFeature.getList2059().iterator();
            while (it.hasNext()) {
                jsonArray2.add(it.next());
            }
            ((JsonObject) jsonObject2).add("commands", (JsonElement) jsonArray2);
            jsonArray.add(jsonObject2);
        }
        jsonObject.add("macros", (JsonElement) jsonArray);
        return jsonObject;
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        if (jsonElement.isJsonObject() && jsonElement.getAsJsonObject().has("macros")) {
            Iterator it = jsonElement.getAsJsonObject().getAsJsonArray("macros").iterator();
            while (it.hasNext()) {
                JsonElement jsonElement2 = (JsonElement) it.next();
                if (jsonElement2.isJsonObject()) {
                    JsonObject asJsonObject = jsonElement2.getAsJsonObject();
                    if (PresetHelperModeHelper.is435(asJsonObject, "name", "type", "key", "mouse", "commands")) {
                        try {
                            Mode_4 mode_4831 = Mode_4.getMode_4831(asJsonObject.get("type").getAsString());
                            KeybindFeature keybindFeature832 = mode_4831.getKeybindFeature832(asJsonObject.get("name").getAsString(), new Keybind(asJsonObject.get("key").getAsInt(), Keybind.KeybindMode.TOGGLE, asJsonObject.get("mouse").getAsBoolean()));
                            Iterator it2 = asJsonObject.get("commands").getAsJsonArray().iterator();
                            while (it2.hasNext()) {
                                keybindFeature832.getList2059().add(((JsonElement) it2.next()).getAsString());
                            }
                            if (mode_4831 == Mode_4.HOLD) {
                                while (keybindFeature832.getList2059().size() > 2) {
                                    keybindFeature832.getList2059().removeLast();
                                }
                            }
                            register(keybindFeature832);
                        } catch (Exception e) {
                        }
                    }
                }
            }
        }
    }

    @Override // me.mioclient.PresetHelper_7
    public String getConfigName() {
        return "macros.json";
    }
}
