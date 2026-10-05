package dev.myth.UI.hud;

import dev.myth.Myth;
import dev.myth.UI.font.FontManager;
import dev.myth.UI.setting.Setting;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import net.minecraft.client.Minecraft;
import net.minecraft.world.entity.EquipmentSlot;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.item.Items;
import net.minecraft.util.Mth;

public final class TextRadar extends LinesHudElement {
    public final Setting<String> sort = addSetting(Setting.choice("sort", "Sort", "Distance", List.of("Distance", "Angle")));
    public final Setting<Boolean> health = addSetting(Setting.bool("health", "Health", true));
    public final Setting<Boolean> distance = addSetting(Setting.bool("distance", "Distance", false));
    public final Setting<Boolean> totemPops = addSetting(Setting.bool("totemPops", "TotemPops", false));
    public final Setting<Boolean> friendColor = addSetting(Setting.bool("friendColor", "FriendColor", true));
    public final Setting<Boolean> enemyColor = addSetting(Setting.bool("enemyColor", "EnemyColor", true));
    public final Setting<Boolean> armor = addSetting(Setting.bool("armor", "Armor", false));
    public final Setting<Boolean> limit = addSetting(Setting.bool("limit", "Limit", false).group());
    public final Setting<Integer> max = addSetting(Setting.integer("max", "Max", 8, 1, 32).visibleWhen(limit::isExpanded));
    public final Setting<Boolean> ignore = addSetting(Setting.bool("ignore", "Ignore", false).group().action());
    public final Setting<Boolean> friends = addSetting(Setting.bool("friends", "Friends", false).visibleWhen(ignore::isExpanded));
    public final Setting<Boolean> nakeds = addSetting(Setting.bool("nakeds", "Nakeds", false).visibleWhen(ignore::isExpanded));
    // Mio's role manager is supplied locally; no unrelated feature modules are registered.
    public final Setting<String> friendNames = addSetting(Setting.text("friendNames", "FriendNames", "").description("Comma-separated player names."));
    public final Setting<String> enemyNames = addSetting(Setting.text("enemyNames", "EnemyNames", "").description("Comma-separated player names."));
    private static final EquipmentSlot[] SLOTS = {EquipmentSlot.HEAD, EquipmentSlot.CHEST, EquipmentSlot.LEGS, EquipmentSlot.FEET};
    public TextRadar() { super("text_radar", "TextRadar", false); }
    @Override public String getDescription() { return "Shows nearby players, health, distance, armor and totem pops."; }
    private boolean role(Player player, Setting<String> names) {
        return java.util.Arrays.stream(names.get().split(",")).anyMatch(name -> name.trim().equalsIgnoreCase(player.getName().getString()));
    }
    private boolean naked(Player player) { for (EquipmentSlot slot : SLOTS) if (!player.getItemBySlot(slot).isEmpty()) return false; return true; }
    private double order(Player player) {
        var client = Minecraft.getInstance();
        if (sort.get().equals("Distance")) return player.distanceToSqr(client.player);
        double yaw = Math.toDegrees(Math.atan2(player.getZ() - client.player.getZ(), player.getX() - client.player.getX())) - 90;
        return Math.abs(Mth.wrapDegrees(yaw - client.gameRenderer.mainCamera().yRot()));
    }
    @Override protected List<Line> getLines(FontManager fonts) {
        var client = Minecraft.getInstance(); if (client.level == null || client.player == null) return List.of();
        return client.level.players().stream().filter(player -> player != client.player || Myth.UI_MANAGER.isHudEditorOpen())
                .filter(player -> (!friends.get() || !role(player, friendNames)) && (!nakeds.get() || !naked(player)))
                .sorted(Comparator.comparingDouble(this::order)).limit(limit.get() ? max.get() : Long.MAX_VALUE).map(player -> {
                    float hp = player.getHealth() + player.getAbsorptionAmount(); StringBuilder text = new StringBuilder();
                    if (health.get()) text.append(healthColor(hp)).append(Math.round(hp)).append(' ');
                    text.append("\u00a7r").append(player.getName().getString());
                    int pops = Myth.HUD_MANAGER.data.pops(player.getUUID());
                    if (totemPops.get() && pops > 0) text.append(pops > 2 ? "\u00a74" : pops > 1 ? "\u00a7c" : "\u00a7e").append(" -").append(pops);
                    if (distance.get()) text.append("\u00a7f ").append(Math.round(client.player.distanceTo(player))).append('m');
                    if (armor.get()) {
                        String gear = "G";
                        for (EquipmentSlot slot : SLOTS) {
                            var item = player.getItemBySlot(slot);
                            if (item.is(Items.ELYTRA)) { gear = "W"; break; }
                            String id = net.minecraft.core.registries.BuiltInRegistries.ITEM.getKey(item.getItem()).getPath();
                            if ((!id.startsWith("diamond_") && !id.startsWith("netherite_")) || !item.isEnchanted()) gear = "";
                        }
                        if (!gear.isEmpty()) text.append("\u00a7f [").append(gear).append(']');
                    }
                    Integer color = null;
                    if (friendColor.get() && role(player, friendNames)) color = 0xff00ffff;
                    else if (enemyColor.get() && role(player, enemyNames)) color = 0xffb40000;
                    return new Line(player.getUUID().toString(), text.toString(), color);
                }).toList();
    }
    private static String healthColor(double hp) { return hp >= 20 ? "\u00a7a" : hp >= 16 ? "\u00a72" : hp >= 10 ? "\u00a76" : hp >= 4 ? "\u00a7c" : "\u00a74"; }
}
