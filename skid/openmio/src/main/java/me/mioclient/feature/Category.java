package me.mioclient.feature;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.file.CopyOption;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Objects;
import java.util.stream.Stream;
import me.mioclient.ArgumentTypeHelper;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.EnumSettingConverter;
import me.mioclient.FontsSearchHelper4_2;
import me.mioclient.PresetEnumSettingHelper;
import me.mioclient.PresetHelperMode;
import me.mioclient.PresetSearchHelper419;
import me.mioclient.PresetSearchHelper419_2;
import net.minecraft.client.gui.screen.Screen;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/feature/Category.class */
public class Category extends FontsSearchHelper4_2 {
    public final PresetEnumSettingHelper presetEnumSettingHelper = new PresetEnumSettingHelper("Category");
    public final Screen screen;

    public Category(Screen screen, Path path) {
        this.screen = screen;
        this.presetEnumSettingHelper.getArrayList1968().add(new PresetSearchHelper419(this.presetEnumSettingHelper, new ArgumentTypeHelper().getArgumentTypeHelper2919(getString2713(path)).getString2921("Preset: \u0001")));
        PresetHelperMode presetHelperMode2715 = getPresetHelperMode2715(path);
        if (presetHelperMode2715 != null) {
            this.presetEnumSettingHelper.register(new PresetSearchHelper419_2(this.presetEnumSettingHelper, "Import", () -> {
                do2714(path, presetHelperMode2715);
            }));
        } else {
            for (PresetHelperMode presetHelperMode : PresetHelperMode.values()) {
                this.presetEnumSettingHelper.register(new PresetSearchHelper419_2(this.presetEnumSettingHelper, EnumSettingConverter.getString913(presetHelperMode), () -> {
                    do2714(path, presetHelperMode);
                }));
            }
        }
        this.presetEnumSettingHelper.register(new PresetSearchHelper419_2(this.presetEnumSettingHelper, "Cancel", this::do1714));
        getArrayList2831().add(this.presetEnumSettingHelper);
    }

    @Override // me.mioclient.FontsSearchHelper4_2
    public void do1714() {
        minecraftClient.setScreen(this.screen);
    }

    public String getString2713(Path path) {
        return path.toFile().getName().replace(".json", "");
    }

    public void do2714(Path path, PresetHelperMode presetHelperMode) {
        try {
            Files.copy(path, presetHelperMode.getPath3012().resolve(path.toFile().getName()), new CopyOption[0]);
        } catch (Exception e) {
            e.printStackTrace();
        }
        BaritoneHelper_3.presetHelper.getPresetHelperSearchHelper4_273(presetHelperMode).do34();
        minecraftClient.setScreen(this.screen);
    }

    public PresetHelperMode getPresetHelperMode2715(Path path) {
        try {
            JsonElement parseString = JsonParser.parseString(Files.readString(path));
            if (!parseString.isJsonObject()) {
                return null;
            }
            JsonObject asJsonObject = parseString.getAsJsonObject();
            if (asJsonObject.has("category")) {
                return PresetHelperMode.getPresetHelperMode3013(asJsonObject.get("category").getAsString());
            }
            return null;
        } catch (Exception e) {
            return null;
        }
    }

    public static boolean is2716(Screen screen, java.util.List<Path> list) {
        ArrayList arrayList = new ArrayList();
        Stream<Path> filter = list.stream().filter(path -> {
            return path.toString().endsWith(".json");
        });
        Objects.requireNonNull(arrayList);
        filter.forEach(arrayList::add);
        if (arrayList.size() != 1) {
            return false;
        }
        minecraftClient.setScreen(new Category(screen, (Path) arrayList.getFirst()));
        return true;
    }
}
