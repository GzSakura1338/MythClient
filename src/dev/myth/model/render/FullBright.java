package dev.myth.model.render;

import dev.myth.UI.setting.Setting;
import dev.myth.event.api.annotations.EventTarget;
import dev.myth.event.impl.PlayerTickEvent;
import dev.myth.model.Category;
import dev.myth.model.Model;
import java.util.List;
import net.minecraft.client.Minecraft;
import net.minecraft.world.effect.MobEffectInstance;
import net.minecraft.world.effect.MobEffects;

/** Starain FullBright, with its Gamma ramp and client-side NightVision mode. */
public final class FullBright extends Model {
    public final Setting<String> mode = addSetting(Setting.choice("mode", "Mode", "Gamma", List.of("Gamma", "NightVision")));
    public final Setting<Integer> brightness = addSetting(Setting.integer("brightness", "Brightness", 15, 1, 15)
            .visibleWhen(() -> mode.get().equals("Gamma")));
    private double gamma;

    public FullBright() {
        super("fullbright", "FullBright", Category.RENDER);
        setDescription("Brightens the world using Gamma or NightVision.");
    }

    @Override protected void onEnable() {
        var options = Minecraft.getInstance().options;
        if (options != null) gamma = options.gamma().get();
    }

    @Override protected void onDisable() {
        var player = Minecraft.getInstance().player;
        if (player != null) player.removeEffect(MobEffects.NIGHT_VISION);
    }

    @EventTarget private void onPlayerTick(PlayerTickEvent event) {
        var player = Minecraft.getInstance().player;
        if (player == null) return;
        if (mode.get().equals("Gamma")) {
            player.removeEffect(MobEffects.NIGHT_VISION);
            if (gamma < brightness.get()) gamma = Math.min(brightness.get(), gamma + .1D);
        } else {
            player.addEffect(new MobEffectInstance(MobEffects.NIGHT_VISION, 1337, 0, false, false, false));
        }
    }

    public boolean isGammaMode() { return isEnabled() && mode.get().equals("Gamma"); }
    public double getGamma() { return gamma; }
}
