package dev.myth.UI.hud;

import dev.myth.UI.font.FontManager;
import dev.myth.UI.setting.Setting;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import net.minecraft.client.Minecraft;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.item.ItemEntity;

public final class EntityList extends LinesHudElement {
    public final Setting<List<String>> whiteList = addSetting(HudSelection.registry("whiteList", "WhiteList", BuiltInRegistries.ENTITY_TYPE, "minecraft:player"));
    public final Setting<List<String>> items = addSetting(HudSelection.registry("items", "Items", BuiltInRegistries.ITEM, "minecraft:obsidian"));
    public final Setting<String> selection = addSetting(Setting.choice("selection", "Selection", "BlackList", HudSelection.MODES));
    public final Setting<String> itemSelection = addSetting(Setting.choice("itemSelection", "ItemSelection", "Any", HudSelection.MODES)
            .visibleWhen(() -> HudSelection.allows(selection.get(), "minecraft:item", whiteList.get())));
    public final Setting<String> sorting = addSetting(Setting.choice("sorting", "Sorting", "Alphabet", List.of("Alphabet", "Count", "Length")));
    public final Setting<Boolean> customNames = addSetting(Setting.bool("customNames", "CustomNames", true));
    public final Setting<Boolean> coloredCount = addSetting(Setting.bool("coloredCount", "ColoredCount", true));
    public EntityList() { super("entity_list", "EntityList", false); }
    @Override public String getDescription() { return "Counts loaded entities and dropped items with registry filters."; }
    @Override protected List<Line> getLines(FontManager fonts) {
        var level = Minecraft.getInstance().level; if (level == null) return List.of();
        java.util.Map<String, Integer> counts = new HashMap<>();
        for (Entity entity : level.entitiesForRendering()) {
            if (!HudSelection.allows(selection.get(), BuiltInRegistries.ENTITY_TYPE.getKey(entity.getType()).toString(), whiteList.get())) continue;
            int count = 1; String name = customNames.get() ? entity.getName().getString() : entity.getType().getDescription().getString();
            if (entity instanceof ItemEntity item) {
                if (item.getItem().isEmpty() || !HudSelection.allows(itemSelection.get(), BuiltInRegistries.ITEM.getKey(item.getItem().getItem()).toString(), items.get())) continue;
                name = item.getItem().getHoverName().getString(); count = item.getItem().getCount();
            }
            counts.merge(name, count, Integer::sum);
        }
        var entries = new ArrayList<>(counts.entrySet());
        entries.sort(switch (sorting.get()) {
            case "Count" -> Comparator.comparingInt(entry -> -entry.getValue());
            case "Length" -> Comparator.comparingDouble(entry -> fonts.width(entry.getKey() + " x" + entry.getValue()));
            default -> java.util.Map.Entry.comparingByKey();
        });
        return entries.stream().map(entry -> new Line(entry.getKey(), entry.getKey() + (coloredCount.get() ? "\u00a7f" : "\u00a7r") + " x" + entry.getValue())).toList();
    }
}
