package dev.myth.UI.hud;

import dev.myth.Myth;
import dev.myth.UI.font.FontManager;
import dev.myth.UI.setting.Setting;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;
import net.minecraft.client.Minecraft;
import net.minecraft.world.level.block.entity.BarrelBlockEntity;
import net.minecraft.world.level.block.entity.ChestBlockEntity;

public final class Metrics extends LinesHudElement {
    public final Setting<Boolean> speed = addSetting(Setting.bool("speed", "Speed", true).group());
    public final Setting<Boolean> bps = addSetting(Setting.bool("bps", "BPS", false).visibleWhen(speed::isExpanded));
    public final Setting<Boolean> tps = addSetting(Setting.bool("tps", "TPS", true));
    public final Setting<Boolean> ping = addSetting(Setting.bool("ping", "Ping", true));
    public final Setting<Boolean> fps = addSetting(Setting.bool("fps", "FPS", true));
    public final Setting<Boolean> serverBrand = addSetting(Setting.bool("serverBrand", "ServerBrand", true));
    public final Setting<Boolean> durability = addSetting(Setting.bool("durability", "Durability", false));
    public final Setting<Boolean> chest = addSetting(Setting.bool("chest", "Chest", false).group());
    public final Setting<Boolean> doubleChest = addSetting(Setting.bool("double", "Double", false).visibleWhen(chest::isExpanded));
    public Metrics() { super("metrics", "Metrics", true); }
    @Override public String getDescription() { return "Shows speed, TPS, ping, FPS, server brand, durability and loaded chests."; }
    @Override protected List<Line> getLines(FontManager fonts) {
        var client = Minecraft.getInstance(); var data = Myth.HUD_MANAGER.data;
        List<Line> lines = new ArrayList<>();
        if (speed.get()) lines.add(new Line("speed", String.format(Locale.ROOT, "Speed \u00a7f%.2f%s", data.speedKmh() / (bps.get() ? 3.6 : 1), bps.get() ? "b/s" : "km/h")));
        if (tps.get()) lines.add(new Line("tps", String.format(Locale.ROOT, "TPS \u00a7f%.2f", data.tps())));
        if (ping.get()) lines.add(new Line("ping", "Ping \u00a7f" + data.ping() + "ms"));
        if (fps.get()) lines.add(new Line("fps", "FPS \u00a7f" + client.getFps()));
        if (serverBrand.get()) lines.add(new Line("brand", "ServerBrand \u00a7f" + (client.getConnection() == null ? "Unknown" : client.getConnection().serverBrand())));
        if (durability.get() && client.player != null && client.player.getMainHandItem().isDamageableItem()) {
            var item = client.player.getMainHandItem(); lines.add(new Line("durability", "Durability \u00a7f" + (item.getMaxDamage() - item.getDamageValue())));
        }
        if (chest.get()) {
            long count = data.blockEntities().stream().filter(e -> e instanceof ChestBlockEntity || e instanceof BarrelBlockEntity).count();
            lines.add(new Line("chest", "Chests \u00a7f" + count / (doubleChest.get() ? 2 : 1)));
        }
        lines.sort(Comparator.comparingDouble(line -> -fonts.width(line.text()))); return lines;
    }
}
