package me.mioclient;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import java.nio.file.Path;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.module.Module;
import me.mioclient.module.client.Colors;
import me.mioclient.module.client.Fonts;
import me.mioclient.module.client.HUD;
import me.mioclient.module.client.Notifications;
import me.mioclient.module.client.UI;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/PresetHelperMode.class */
public enum PresetHelperMode implements EnumSettingHelper, PresetHelper_7 {
    MODULES("Modules") { // from class: me.mioclient.PresetHelperMode.Inner_3
        @Override // me.mioclient.PresetHelper_7
        public JsonElement toJson() {
            JsonObject jsonObject = new JsonObject();
            do3014(jsonObject);
            for (Module module : BaritoneHelper_3.keyPearlSearchHelper4.getRegistry()) {
                if (!(module instanceof ModuleList) && !(module instanceof Fonts)) {
                    JsonObject jsonObject2 = new JsonObject();
                    jsonObject2.addProperty("enabled", Boolean.valueOf(module.isToggled()));
                    JsonObject jsonObject3 = new JsonObject();
                    for (Setting<?> setting : module.getRegistry()) {
                        if (!(setting instanceof ColorSetting) && !setting.is2352() && !setting.is2354()) {
                            jsonObject3.add(setting.getConfigName(), setting.toJson());
                        }
                    }
                    if (!PresetHelperModeHelper.is436(jsonObject3)) {
                        jsonObject2.add("settings", (JsonElement) jsonObject3);
                    }
                    jsonObject.add(module.getConfigName(), (JsonElement) jsonObject2);
                }
            }
            return jsonObject;
        }

        @Override // me.mioclient.PresetHelperMode, me.mioclient.PresetHelper_7
        public void fromJson(JsonElement jsonElement) {
            PresetHelperModeHelper_2.presetHelperModeHelper_2.do2095(jsonElement.getAsJsonObject());
            super.fromJson(jsonElement);
        }
    },
    BINDS("Binds") { // from class: me.mioclient.PresetHelperMode.Inner_4
        @Override // me.mioclient.PresetHelper_7
        public JsonElement toJson() {
            JsonObject jsonObject = new JsonObject();
            do3014(jsonObject);
            for (Module module : BaritoneHelper_3.keyPearlSearchHelper4.getRegistry()) {
                JsonElement jsonObject2 = new JsonObject();
                ((JsonObject) jsonObject2).addProperty("key", Integer.valueOf(module.getKeybind().get1945()));
                ((JsonObject) jsonObject2).addProperty("state", module.getKeybind().getKeybindMode1946().name().toLowerCase());
                ((JsonObject) jsonObject2).addProperty("mouse", Boolean.valueOf(module.getKeybind().is1947()));
                jsonObject.add(module.getConfigName(), jsonObject2);
            }
            return jsonObject;
        }

        @Override // me.mioclient.PresetHelperMode, me.mioclient.PresetHelper_7
        public void fromJson(JsonElement jsonElement) {
            PresetHelperModeHelper_2.presetHelperModeHelper_2.do2097(jsonElement.getAsJsonObject());
            super.fromJson(jsonElement);
        }
    },
    COLORS("Colors") { // from class: me.mioclient.PresetHelperMode.Inner_5
        @Override // me.mioclient.PresetHelper_7
        public JsonElement toJson() {
            JsonObject jsonObject = new JsonObject();
            do3014(jsonObject);
            for (Module module : BaritoneHelper_3.keyPearlSearchHelper4.getRegistry()) {
                JsonElement jsonObject2 = new JsonObject();
                JsonObject jsonObject3 = new JsonObject();
                for (Setting<?> setting : module.getRegistry()) {
                    if ((setting instanceof ColorSetting) && !setting.is2354()) {
                        jsonObject3.add(setting.getConfigName(), setting.toJson());
                    }
                }
                ((JsonObject) jsonObject2).add("settings", (JsonElement) jsonObject3);
                if (!PresetHelperModeHelper.is436((JsonObject) jsonObject2)) {
                    jsonObject.add(module.getConfigName(), jsonObject2);
                }
            }
            return jsonObject;
        }

        @Override // me.mioclient.PresetHelperMode, me.mioclient.PresetHelper_7
        public void fromJson(JsonElement jsonElement) {
            PresetHelperModeHelper_2.presetHelperModeHelper_2.do2096(jsonElement.getAsJsonObject(), module -> {
                return false;
            });
            super.fromJson(jsonElement);
        }
    },
    VISUALS("Visuals") { // from class: me.mioclient.PresetHelperMode.Inner_6
        public static final Set<Class<? extends Module>> set = new HashSet(List.of(UI.class, Fonts.class, HUD.class, Colors.class, Notifications.class));

        @Override // me.mioclient.PresetHelper_7
        public JsonElement toJson() {
            JsonObject jsonObject = new JsonObject();
            do3014(jsonObject);
            for (Module module : BaritoneHelper_3.keyPearlSearchHelper4.getRegistry()) {
                JsonObject jsonObject2 = new JsonObject();
                boolean is163 = is163(module);
                if (is163) {
                    jsonObject2.addProperty("enabled", Boolean.valueOf(module.isToggled()));
                }
                JsonObject jsonObject3 = new JsonObject();
                for (Setting<?> setting : module.getRegistry()) {
                    if (!setting.is2354() && ((setting instanceof ColorSetting) || is163)) {
                        jsonObject3.add(setting.getConfigName(), setting.toJson());
                    }
                }
                jsonObject2.add("settings", (JsonElement) jsonObject3);
                if (!PresetHelperModeHelper.is436(jsonObject2)) {
                    jsonObject.add(module.getConfigName(), (JsonElement) jsonObject2);
                }
            }
            return jsonObject;
        }

        @Override // me.mioclient.PresetHelperMode, me.mioclient.PresetHelper_7
        public void fromJson(JsonElement jsonElement) {
            PresetHelperModeHelper_2.presetHelperModeHelper_2.do2096(jsonElement.getAsJsonObject(), this::is163);
            super.fromJson(jsonElement);
        }

        public boolean is163(Module module) {
            if (module instanceof ModuleList) {
                return true;
            }
            return set.contains(module.getClass()) || module.getCategory() == Category.RENDER || module.getCategory() == Category.HUD;
        }
    },
    MACRO("Macro") { // from class: me.mioclient.PresetHelperMode.Inner
        @Override // me.mioclient.PresetHelper_7
        public JsonElement toJson() {
            return BaritoneHelper_3.searchHelper4_12.toJson();
        }

        @Override // me.mioclient.PresetHelperMode, me.mioclient.PresetHelper_7
        public void fromJson(JsonElement jsonElement) {
            synchronized (BaritoneHelper_3.searchHelper4_12.getRegistry()) {
                BaritoneHelper_3.searchHelper4_12.getRegistry().clear();
                BaritoneHelper_3.searchHelper4_12.fromJson(jsonElement);
            }
        }
    },
    ALL("All") { // from class: me.mioclient.PresetHelperMode.Inner_2
        @Override // me.mioclient.PresetHelper_7
        public JsonElement toJson() {
            return BaritoneHelper_3.keyPearlSearchHelper4.toJson().getAsJsonObject();
        }
    };

    public final String name;
    public final Path path;

    PresetHelperMode(String str) {
        this.name = str;
        this.path = PresetHelper.path2.resolve(str.toLowerCase());
    }

    public Path getPath3012() {
        return this.path;
    }

    public static PresetHelperMode getPresetHelperMode3013(String str) {
        for (PresetHelperMode presetHelperMode : values()) {
            if (presetHelperMode.getName().equalsIgnoreCase(str)) {
                return presetHelperMode;
            }
        }
        return null;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        BaritoneHelper_3.keyPearlSearchHelper4.fromJson(jsonElement);
    }

    public void do3014(JsonObject jsonObject) {
        jsonObject.addProperty("client", "mio-fabric");
        jsonObject.addProperty("category", getName().toLowerCase(Locale.ROOT));
    }
}
