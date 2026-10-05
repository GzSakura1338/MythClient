package me.mioclient.module.render;

import me.mioclient.PhaseESPHelper;
import me.mioclient.ViewModelFontsSearchHelper42;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.module.Module;
import net.minecraft.util.Hand;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/render/ViewModel.class */
public class ViewModel extends Module {
    public static final String string = "°";
    public Setting<Boolean> adjust;
    public Setting<Boolean> shadow;
    public Setting<Boolean> noSway;
    public Setting<Boolean> instantSwap;
    public Setting<Boolean> eating;
    public Setting<Boolean> mainHand2;
    public Setting<Boolean> arm;
    public Setting<Float> mainX;
    public Setting<Float> mainY;
    public Setting<Float> mainZ;
    public Setting<Float> mainScaleX;
    public Setting<Float> mainScaleY;
    public Setting<Float> mainScaleZ;
    public Setting<Float> mainRotateX;
    public Setting<Float> mainRotateY;
    public Setting<Float> mainRotateZ;
    public Setting<Boolean> offHand;
    public Setting<Float> offX;
    public Setting<Float> offY;
    public Setting<Float> offZ;
    public Setting<Float> offScaleX;
    public Setting<Float> offScaleY;
    public Setting<Float> offScaleZ;
    public Setting<Float> offRotateX;
    public Setting<Float> offRotateY;
    public Setting<Float> offRotateZ;
    public Setting<Boolean> misc;
    public Setting<Float> eatMultiplier;
    public Setting<Boolean> noTridentAnim;
    public Setting<Boolean> swingProgress;
    public Setting<Boolean> static_;
    public Setting<Boolean> mainHand;
    public Setting<Float> swingProgressAmount2;
    public Setting<Boolean> offHand2;
    public Setting<Float> swingProgressAmount;
    public Setting<Boolean> viewModelFov;
    public Setting<Integer> fovAmount;

    public ViewModel() {
        super("ViewModel", "Transforms your 1st person view model.", Category.RENDER, new String[0]);
        PhaseESPHelper.do1351(this);
        this.mainHand.do2329("SwingMainHand");
        this.offHand2.do2329("SwingOffHand");
        this.swingProgressAmount.do2329("SwingProgressAmountOffHand");
        this.adjust.do2339(() -> {
            if (this.adjust.getValue().booleanValue()) {
                minecraftClient.setScreen(new ViewModelFontsSearchHelper42(this));
                this.adjust.do2333(false);
            }
        });
    }

    public float get3142(Hand hand, float f) {
        if (!isToggled() || !this.swingProgress.getValue().booleanValue()) {
            return f;
        }
        if (hand == Hand.MAIN_HAND && this.mainHand.getValue().booleanValue()) {
            return Math.max(this.static_.getValue().booleanValue() ? 0.0f : f, this.swingProgressAmount2.getValue().floatValue());
        }
        if (hand == Hand.OFF_HAND && this.offHand2.getValue().booleanValue()) {
            return Math.max(this.static_.getValue().booleanValue() ? 0.0f : f, this.swingProgressAmount.getValue().floatValue());
        }
        return f;
    }
}
