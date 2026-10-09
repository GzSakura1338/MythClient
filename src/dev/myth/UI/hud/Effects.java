package dev.myth.UI.hud;

import dev.myth.UI.font.FontManager;
import dev.myth.UI.setting.Selection;
import dev.myth.UI.setting.Setting;
import dev.myth.model.render.NoRender;
import java.util.Comparator;
import java.util.List;
import net.minecraft.client.Minecraft;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.world.effect.MobEffectUtil;
import net.minecraft.world.effect.MobEffects;

public final class Effects extends LinesHudElement {
    public final Setting<Boolean> vanilla = addSetting(Setting.bool("vanilla", "Vanilla", false));
    public final Setting<String> selection = addSetting(Setting.choice("selection", "Selection", "Any", Selection.MODES));
    public final Setting<List<String>> whiteList = addSetting(Selection.registry("whiteList", "WhiteList", BuiltInRegistries.MOB_EFFECT));
    public Effects() { super("effects", "Effects", true); }
    @Override public String getDescription() { return "Lists active potion effects and their remaining duration."; }
    @Override protected List<Line> getLines(FontManager fonts) {
        var client = Minecraft.getInstance();
        if (client.player == null) return List.of();
        // Mio's HUD hides Darkness here too, so the list matches what NoRender lets through.
        NoRender noRender = NoRender.active();
        boolean hideDarkness = noRender != null && noRender.darkness.get();
        return client.player.getActiveEffects().stream()
                .filter(effect -> !hideDarkness || !effect.getEffect().equals(MobEffects.DARKNESS))
                .filter(effect -> Selection.allows(selection.get(),
                BuiltInRegistries.MOB_EFFECT.getKey(effect.getEffect().value()).toString(), whiteList.get()))
                .sorted(Comparator.comparing(effect -> effect.getEffect().value().getDisplayName().getString()))
                .map(effect -> {
                    String id = BuiltInRegistries.MOB_EFFECT.getKey(effect.getEffect().value()).toString();
                    String duration = MobEffectUtil.formatDuration(effect, 1, client.level.tickRateManager().tickrate()).getString();
                    if (duration.startsWith("0")) duration = duration.substring(1);
                    String text = effect.getEffect().value().getDisplayName().getString() + " "
                            + (effect.getAmplifier() == 0 ? "" : (effect.getAmplifier() + 1) + " ") + "\u00a7f" + duration;
                    return new Line(id, text, vanilla.get() ? 0xff000000 | effect.getEffect().value().getColor() : null);
                }).toList();
    }
}
