package me.mioclient.module;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import java.util.function.Function;
import me.mioclient.ArgumentTypeHelper;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.HUDHelper;
import me.mioclient.Helper_9;
import me.mioclient.PresetHelper_7;
import me.mioclient.SearchHelper_4;
import me.mioclient.api.Category;
import me.mioclient.api.Keybind;
import me.mioclient.api.Setting;
import me.mioclient.event.EnableEvent;
import net.minecraft.util.Formatting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/Module.class */
public class Module extends Feature implements SearchHelper_4, HUDHelper, Helper_9<Setting<?>, List<Setting<?>>>, PresetHelper_7 {
    public final Category category;
    public final List<Setting<?>> registry;
    public final String[] aliases;
    public Keybind keybind;
    public boolean toggled;
    public boolean drawn;
    public boolean wip;

    public Module(String str, String str2, Category category, String... strArr) {
        super(str);
        this.registry = new ArrayList();
        this.keybind = Keybind.keybind;
        this.drawn = true;
        this.category = category;
        setDescription(str2);
        String[] strArr2 = (String[]) Objects.requireNonNullElseGet(strArr, () -> {
            return new String[0];
        });
        this.aliases = (String[]) Arrays.copyOf(strArr2, strArr2.length + 1);
        this.aliases[strArr2.length] = str;
    }

    public Module(String str, Category category, String... strArr) {
        this(str, "", category, strArr);
    }

    public Category getCategory() {
        return this.category;
    }

    public Keybind getKeybind() {
        return this.keybind;
    }

    public String getInfoString() {
        String str;
        String string397 = BaritoneHelper_3.notificationsHelper.getString397(this);
        if (getInfo() != null) {
            str = new ArgumentTypeHelper().getArgumentTypeHelper2919(String.valueOf(Formatting.GRAY)).getArgumentTypeHelper2919(getInfo()).getArgumentTypeHelper2919(String.valueOf(Formatting.WHITE)).getArgumentTypeHelper2919(String.valueOf(Formatting.GRAY)).getString2921("\u0001 [\u0001\u0001\u0001]");
        } else {
            str = "";
        }
        return new ArgumentTypeHelper().getArgumentTypeHelper2919(str).getArgumentTypeHelper2919(string397).getString2921("\u0001\u0001");
    }

    public void setKeybind(Keybind keybind) {
        this.keybind = keybind;
    }

    public void modifyKeybind(Function<Keybind, Keybind> function) {
        this.keybind = function.apply(this.keybind);
    }

    /* JADX WARN: Can't rename method to resolve collision */
    @Override // me.mioclient.Helper_9
    public List<Setting<?>> getRegistry() {
        return this.registry;
    }

    @Override // me.mioclient.Helper_9
    public boolean register(Setting<?> setting) {
        return this.registry.add(setting);
    }

    @Override // me.mioclient.Helper_9
    public boolean unregister(Setting<?> setting) {
        return this.registry.remove(setting);
    }

    public <T> Setting<T> add(Setting<T> setting) {
        register((Setting<?>) setting);
        return setting;
    }

    public <T> Setting<T> add(Setting<T> setting, Setting<?> setting2) {
        this.registry.add(this.registry.indexOf(setting2) + 1, setting);
        return setting;
    }

    public boolean isToggled() {
        return this.toggled;
    }

    @Override // me.mioclient.HUDHelper
    public void enable() {
        if (isToggled()) {
            return;
        }
        this.toggled = true;
        baritoneHelper.do1796(this);
        baritoneHelper.getObject1794(new EnableEvent(this));
        onToggle();
        onEnable();
    }

    public void disable() {
        if (isToggled()) {
            baritoneHelper.do1802(this);
            this.toggled = false;
            baritoneHelper.getObject1794(new EnableEvent(this));
            onToggle();
            onDisable();
        }
    }

    public void onEnable() {
    }

    public void onDisable() {
    }

    public void onToggle() {
    }

    public String getInfo() {
        return null;
    }

    public boolean isDrawn() {
        return this.drawn;
    }

    public void setDrawn(boolean z) {
        this.drawn = z;
    }

    public String[] getAliases() {
        return this.aliases;
    }

    public JsonElement toJson() {
        JsonObject jsonObject = new JsonObject();
        jsonObject.addProperty("enabled", Boolean.valueOf(this.toggled));
        jsonObject.addProperty("key", Integer.valueOf(this.keybind.get1945()));
        jsonObject.addProperty("state", this.keybind.getKeybindMode1946().getString2552());
        jsonObject.addProperty("mouse", Boolean.valueOf(this.keybind.is1947()));
        jsonObject.addProperty("drawn", Boolean.valueOf(this.drawn));
        JsonObject jsonObject2 = new JsonObject();
        for (Setting<?> setting : getRegistry()) {
            if (!setting.is2352() && !setting.is2354()) {
                jsonObject2.add(setting.getConfigName(), setting.toJson());
            }
        }
        jsonObject.add("settings", (JsonElement) jsonObject2);
        return jsonObject;
    }

    public void fromJson(JsonElement jsonElement) {
        if (jsonElement instanceof JsonObject) {
            JsonObject jsonObject = (JsonObject) jsonElement;
            if (jsonObject.has("settings")) {
                JsonObject jsonObject2 = (jsonObject.get("settings")) instanceof JsonObject ? (JsonObject) (jsonObject.get("settings")) : null;
                if (jsonObject2 instanceof JsonObject) {
                    JsonObject jsonObject3 = jsonObject2;
                    for (Setting<?> setting : getRegistry()) {
                        if (!setting.is2354()) {
                            if (jsonObject3.has(setting.getConfigName())) {
                                try {
                                    setting.fromJson(jsonObject3.get(setting.getConfigName()));
                                } catch (Exception e) {
                                }
                            }
                        }
                    }
                }
            }
            if (jsonObject.has("drawn")) {
                setDrawn(jsonObject.get("drawn").getAsBoolean());
            }
            if (jsonObject.has("key")) {
                modifyKeybind(keybind -> {
                    return keybind.getKeybind1941(jsonObject.get("key").getAsInt()).getKeybind1942(jsonObject.has("state") ? Keybind.KeybindMode.getKeybindMode2553(jsonObject.get("state").getAsString()) : Keybind.KeybindMode.TOGGLE);
                });
            }
            if (jsonObject.has("mouse")) {
                modifyKeybind(keybind2 -> {
                    return keybind2.getKeybind1943(jsonObject.get("mouse").getAsBoolean());
                });
            }
            if (jsonObject.has("enabled")) {
                do495(jsonObject.get("enabled").getAsBoolean());
            }
        }
    }

    @Override // me.mioclient.PresetHelper_7
    public String getConfigName() {
        return getName();
    }

    public boolean isWip() {
        return this.wip;
    }

    public void setWip(boolean z) {
        this.wip = z;
    }
}
