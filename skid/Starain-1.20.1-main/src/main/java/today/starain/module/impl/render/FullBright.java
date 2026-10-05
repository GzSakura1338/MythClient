package today.starain.module.impl.render;

import net.minecraft.world.effect.MobEffectInstance;
import net.minecraft.world.effect.MobEffects;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.PlayerTickEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.value.impl.ModeValue;
import today.starain.value.impl.NumberValue;

public final class FullBright extends ClientModule {
    private final ModeValue mode = new ModeValue("Mode", "Gamma", new String[]{"Gamma", "NightVision"}, this);
    private final NumberValue brightness = new NumberValue("Brightness", 1.0, 15.0, 15.0, 1.0, this, () -> mode.isMode("Gamma"));
    private double gamma;

    public FullBright() {
        setName("FullBright");
        setCategory(Category.RENDER);
    }

    @Override
    protected void onEnable() {
        if (mc.options != null) {
            gamma = mc.options.gamma().get();
        }
    }

    @Override
    protected void onDisable() {
        if (mc.player != null) {
            mc.player.removeEffect(MobEffects.NIGHT_VISION);
        }
    }

    @EventTarget
    public void onPlayerTick(PlayerTickEvent event) {
        if (mc.player == null) {
            return;
        }

        if (mode.isMode("Gamma")) {
            mc.player.removeEffect(MobEffects.NIGHT_VISION);
            double targetBrightness = brightness.getValue();
            if (gamma < targetBrightness) {
                gamma = Math.min(targetBrightness, gamma + 0.1D);
            }
            return;
        }

        mc.player.addEffect(new MobEffectInstance(MobEffects.NIGHT_VISION, 1337, 0, false, false, false));
    }

    public boolean isGammaMode() {
        return isEnabled() && mode.isMode("Gamma");
    }

    public double getGamma() {
        return gamma;
    }

    @Override
    public String getSuffix() {
        return mode.get();
    }
}
