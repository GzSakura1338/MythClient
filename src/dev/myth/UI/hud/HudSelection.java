package dev.myth.UI.hud;

import dev.myth.UI.setting.Setting;
import java.util.List;
import net.minecraft.core.Registry;
import net.minecraft.resources.Identifier;

final class HudSelection {
    static final List<String> MODES = List.of("Any", "WhiteList", "BlackList");
    private HudSelection() {}
    static <T> Setting<List<String>> registry(String id, String name, Registry<T> registry, String... defaults) {
        return Setting.selection(id, name, List.of(defaults), registry.keySet().stream().map(Identifier::toString).sorted().toList());
    }
    static boolean allows(String mode, String id, List<String> list) {
        return switch (mode) { case "WhiteList" -> list.contains(id); case "BlackList" -> !list.contains(id); default -> true; };
    }
}
