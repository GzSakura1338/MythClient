package me.mioclient;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import it.unimi.dsi.fastutil.objects.Object2ObjectOpenHashMap;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.stream.Collectors;
import me.mioclient.MatrixStackEvent;
import me.mioclient.api.Category;
import me.mioclient.api.Keybind;
import me.mioclient.event.KeyEvent;
import me.mioclient.event.Listen;
import me.mioclient.module.Delay;
import me.mioclient.module.Module;
import me.mioclient.module.client.Baritone;
import me.mioclient.module.movement.ObstaclePasser;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/KeyPearlSearchHelper4.class */
public final class KeyPearlSearchHelper4 extends StashFinderModuleListHelper<Module> implements SearchHelper_4, PresetHelper_7 {
    public final Object2ObjectOpenHashMap<Class<? extends Module>, Module> object2ObjectOpenHashMap = new Object2ObjectOpenHashMap<>();

    public KeyPearlSearchHelper4() {
        baritoneHelper.do1796(this);
        PhaseESPHelper.do1352(this);
        do112();
        do111();
        ((List<Module>) this.registry).sort(Comparator.comparing((v0) -> {
            return v0.getName();
        }));
    }

    public void do111() {
    }

    public void do112() {
        if (BaritoneHelper_3.obstaclePasserHelper.is709()) {
            register(new Baritone());
            register(new ObstaclePasser());
        }
    }

    @Listen
    public void onKey(KeyEvent keyEvent) {
        for (Module module : (List<Module>) this.registry) {
            if (module.getKeybind().is1947() == keyEvent.is2588() && module.getKeybind().get1945() == keyEvent.get2587() && module.getKeybind().getKeybindMode1946() == Keybind.KeybindMode.TOGGLE) {
                module.do496();
            }
        }
    }

    @Listen(get219= Helper_7.num5)
    public void onEvent(MatrixStackEvent.Inner_3 inner_3) {
        for (Module module : (List<Module>) this.registry) {
            Keybind keybind = module.getKeybind();
            boolean z = keybind.getKeybindMode1946() == Keybind.KeybindMode.HOLD;
            if (!keybind.is1944() && keybind.getKeybindMode1946() != Keybind.KeybindMode.TOGGLE && (minecraftClient.currentScreen == null || module.isToggled() == z)) {
                module.do495(z == EntityControlSearchHelper4.is2604(keybind));
            }
        }
    }

    public List<Module> getList113(Category category) {
        return (List) ((List<Module>) this.registry).stream().filter(module -> {
            return module.getCategory() == category;
        }).collect(Collectors.toList());
    }

    public List<Delay> getList114() {
        ArrayList arrayList = new ArrayList();
        ((List) this.registry).forEach(module -> {
            if (module instanceof Delay) {
                arrayList.add((Delay) module);
            }
        });
        return arrayList;
    }

    @Override // me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        JsonObject jsonObject = new JsonObject();
        jsonObject.addProperty("client", "mio-fabric");
        jsonObject.addProperty("category", "all");
        for (Module module : getRegistry()) {
            if (!(module instanceof ModuleList)) {
                try {
                    jsonObject.add(module.getConfigName(), module.toJson());
                } catch (Exception e) {
                }
            }
        }
        return jsonObject;
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        for (Module module : getRegistry()) {
            if (!(module instanceof ModuleList)) {
                try {
                    module.fromJson(jsonElement.getAsJsonObject().get(module.getConfigName()));
                } catch (Exception e) {
                }
            }
        }
    }

    @Override // me.mioclient.PresetHelper_7
    public String getConfigName() {
        return "modules.json";
    }

    @Override // me.mioclient.StashFinderModuleListHelper, me.mioclient.Helper_9
    /* renamed from: is115, reason: merged with bridge method [inline-methods] */
    public boolean register(Module module) {
        if (EnumSettingConverterHelper.is1629(module.getClass())) {
            return false;
        }
        EnumSettingConverterHelper.do1628(module);
        this.object2ObjectOpenHashMap.put(module.getClass(), module);
        return super.register(module);
    }

    @Override // me.mioclient.StashFinderModuleListHelper, me.mioclient.Helper_9
    /* renamed from: is116, reason: merged with bridge method [inline-methods] */
    public boolean unregister(Module module) {
        this.object2ObjectOpenHashMap.remove(module.getClass(), module);
        return super.unregister(module);
    }

    @Deprecated
    public <T extends Module> T getModule117(Class<T> cls) {
        return (T) this.object2ObjectOpenHashMap.get(cls);
    }
}
