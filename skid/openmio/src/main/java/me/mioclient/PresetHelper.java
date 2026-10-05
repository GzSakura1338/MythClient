package me.mioclient;

import com.google.gson.JsonParser;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import net.fabricmc.loader.api.FabricLoader;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/PresetHelper.class */
public final class PresetHelper implements SearchHelper_4 {
    public static final Path path = FabricLoader.getInstance().getGameDir().resolve("mio-fabric");
    public static final Path path2 = path.resolve("presets");
    public static final Path path3 = path.resolve("spammer");
    public static final Path path4 = path.resolve("autoez");
    public static final Path path5 = path.resolve("textures");
    public static final Path path6 = path.resolve("maps");
    public static final Path path7 = path.resolve("chunks");
    public static final Path path8 = path.resolve("sounds");
    public static final Path path9 = Path.of(System.getProperty("user.home"), "_____a");
    public static final List<Path> list = new ArrayList();
    public final List<PresetHelper_7> list2 = List.<PresetHelper_7>of(new PresetHelper_7[]{BaritoneHelper_3.discordNotifsHelper, BaritoneHelper_3.keyPearlSearchHelper4, BaritoneHelper_3.searchHelper4_14, BaritoneHelper_3.waypointsSearchHelper4, BaritoneHelper_3.searchHelper4_12, BaritoneHelper_3.searchHelper4_15, BaritoneHelper_3.searchHelper4_20, BaritoneHelper_3.nameTagsSearchHelper4, BaritoneHelper_3.chestStealerSearchHelper4_3, BaritoneHelper_3.notificationsHelper, BaritoneHelper_3.chatFilterSearchHelper4, BaritoneHelper_3.items, BaritoneHelper_3.chatFilterSearchHelper4_2});
    public final Map<PresetHelperMode, PresetHelperSearchHelper4_2> map = new HashMap();
    public final PresetHelper_6 presetHelper_6 = new PresetHelper_6();
    public final PresetHelper_3 presetHelper_3 = new PresetHelper_3();

    public void do71() {
        list.stream().filter(path10 -> {
            return !path10.toFile().exists();
        }).forEachOrdered(path11 -> {
            path11.toFile().mkdir();
        });
        this.presetHelper_6.do72();
    }

    public void do72() {
        list.addAll(Arrays.asList(path, path2, path3, path4, path5, path6, path8));
        list.add(path7);
        for (PresetHelperMode presetHelperMode : PresetHelperMode.values()) {
            list.add(presetHelperMode.getPath3012());
        }
        do71();
        try {
            PresetHelperSearchHelper4.run();
        } catch (Throwable th) {
        }
        for (PresetHelper_7 presetHelper_7 : this.list2) {
            try {
                Path resolve = path.resolve(presetHelper_7.getConfigName());
                if (resolve.toFile().exists()) {
                    presetHelper_7.fromJson(JsonParser.parseString(PresetHelper_4.getString1570(resolve)).getAsJsonObject());
                }
            } catch (Throwable th2) {
                th2.printStackTrace();
            }
        }
        for (PresetHelperMode presetHelperMode2 : PresetHelperMode.values()) {
            PresetHelperSearchHelper4_2 presetHelperSearchHelper4_2 = new PresetHelperSearchHelper4_2(presetHelperMode2);
            this.map.put(presetHelperMode2, presetHelperSearchHelper4_2);
            presetHelperSearchHelper4_2.do34();
        }
    }

    public void do41() {
        if (PresetHelperFeature.flag) {
            return;
        }
        for (PresetHelper_7 presetHelper_7 : this.list2) {
            try {
                PresetHelper_4.do1567(path.resolve(presetHelper_7.getConfigName()), gson.toJson(presetHelper_7.toJson().getAsJsonObject()));
            } catch (Throwable th) {
                th.printStackTrace();
            }
        }
    }

    public PresetHelperSearchHelper4_2 getPresetHelperSearchHelper4_273(PresetHelperMode presetHelperMode) {
        return this.map.get(presetHelperMode);
    }

    public PresetHelper_3 getPresetHelper_374() {
        return this.presetHelper_3;
    }
}
