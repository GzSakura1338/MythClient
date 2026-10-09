package dev.myth.UI.setting;

import java.util.List;
import net.minecraft.core.Registry;
import net.minecraft.resources.Identifier;

/** Mio ScaffoldMode_2: a registry id set plus its Any / WhiteList / BlackList mode. */
public final class Selection {
    public static final List<String> MODES = List.of("Any", "WhiteList", "BlackList");
    private Selection() {}
    public static <T> Setting<List<String>> registry(String id, String name, Registry<T> registry, String... defaults) {
        return Setting.selection(id, name, List.of(defaults), registry.keySet().stream().map(Identifier::toString).sorted().toList());
    }
    public static boolean allows(String mode, String id, List<String> list) {
        return switch (mode) { case "WhiteList" -> list.contains(id); case "BlackList" -> !list.contains(id); default -> true; };
    }
}
