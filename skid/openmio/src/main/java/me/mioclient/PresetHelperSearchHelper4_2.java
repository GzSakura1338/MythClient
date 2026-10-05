package me.mioclient;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.File;
import java.io.IOException;
import java.nio.file.Path;
import java.util.Iterator;
import java.util.List;
import java.util.Objects;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/PresetHelperSearchHelper4_2.class */
public final class PresetHelperSearchHelper4_2 extends StashFinderModuleListHelper<PresetSearchHelper4> implements SearchHelper_4 {
    public static final String string = ".json";
    public final PresetHelperMode presetHelperMode;
    public final Path path;

    public PresetHelperSearchHelper4_2(PresetHelperMode presetHelperMode) {
        this.path = presetHelperMode.getPath3012();
        this.presetHelperMode = presetHelperMode;
    }

    public void do34() {
        ((List) this.registry).clear();
        try {
            for (File file : (File[]) Objects.requireNonNull(this.path.toFile().listFiles())) {
                if (file.getName().endsWith(".json") && file.length() > 0) {
                    try {
                        JsonObject asJsonObject = JsonParser.parseString(PresetHelper_4.getString1570(file.toPath())).getAsJsonObject();
                        if (asJsonObject != null) {
                            ((List) this.registry).add(new PresetSearchHelper4(file.getName().replace(".json", ""), asJsonObject));
                        }
                    } catch (Throwable th) {
                        th.printStackTrace();
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public boolean is35(String str) {
        synchronized (getRegistry()) {
            for (PresetSearchHelper4 presetSearchHelper4 : getRegistry()) {
                if (presetSearchHelper4.getName().equalsIgnoreCase(str)) {
                    this.presetHelperMode.fromJson(presetSearchHelper4.getJsonObject2741());
                    return true;
                }
            }
            return false;
        }
    }

    public boolean is36(String str) {
        this.path.resolve(new ArgumentTypeHelper().getArgumentTypeHelper2919(str).getString2921("\u0001.json")).toFile().delete();
        return ((List<PresetSearchHelper4>) this.registry).removeIf(presetSearchHelper4 -> {
            return presetSearchHelper4.getName().equalsIgnoreCase(str);
        });
    }

    public boolean is37(String str, String str2) {
        for (PresetSearchHelper4 presetSearchHelper4 : (List<PresetSearchHelper4>) this.registry) {
            if (presetSearchHelper4.getName().equalsIgnoreCase(str)) {
                try {
                    do42(this.path, str2, gson.toJson(presetSearchHelper4.getJsonObject2741()));
                    ((List) this.registry).add(new PresetSearchHelper4(str2, presetSearchHelper4.getJsonObject2741()));
                    return true;
                } catch (Exception e) {
                    e.printStackTrace();
                    return false;
                }
            }
        }
        return false;
    }

    public boolean is38(String str, String str2) {
        if (str.equalsIgnoreCase(str2)) {
            return false;
        }
        if (this.path.resolve(new ArgumentTypeHelper().getArgumentTypeHelper2919(str).getString2921("\u0001.json")).toFile().exists()) {
            this.path.resolve(new ArgumentTypeHelper().getArgumentTypeHelper2919(str).getString2921("\u0001.json")).toFile().renameTo(this.path.resolve(new ArgumentTypeHelper().getArgumentTypeHelper2919(str2).getString2921("\u0001.json")).toFile());
        }
        if (is37(str, str2)) {
            if (((List<PresetSearchHelper4>) this.registry).removeIf(presetSearchHelper4 -> {
                return presetSearchHelper4.getName().equalsIgnoreCase(str);
            })) {
                return true;
            }
        }
        return false;
    }

    public void do39(String str) {
        ((List<PresetSearchHelper4>) this.registry).removeIf(presetSearchHelper4 -> {
            return presetSearchHelper4.getName().equalsIgnoreCase(str);
        });
        PresetSearchHelper4 presetSearchHelper42 = new PresetSearchHelper4(str, this.presetHelperMode.toJson().getAsJsonObject());
        presetSearchHelper42.do2740(this.path);
        ((List) this.registry).add(presetSearchHelper42);
    }

    public void do40(String str) {
        for (PresetSearchHelper4 presetSearchHelper4 : (List<PresetSearchHelper4>) this.registry) {
            if (presetSearchHelper4.getString333().equals(str)) {
                presetSearchHelper4.do2740(this.path);
            }
        }
    }

    public void do41() {
        Iterator it = ((List) this.registry).iterator();
        while (it.hasNext()) {
            try {
                ((PresetSearchHelper4) it.next()).do2740(this.path);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public void do42(Path path, String str, String str2) {
        PresetHelper_4.do1567(path.resolve(str), str2);
    }
}
