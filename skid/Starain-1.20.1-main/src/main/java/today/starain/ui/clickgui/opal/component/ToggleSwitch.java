package today.starain.ui.clickgui.opal.component;

import today.starain.ui.clickgui.opal.OpalTheme;
import today.starain.ui.clickgui.opal.render.OpalDraw;
import today.starain.utils.animation.Animation;
import today.starain.utils.animation.Easing;
import today.starain.utils.render.skia.Skia;

import java.util.function.BooleanSupplier;

public final class ToggleSwitch {

    private final Runnable toggleAction;
    private final BooleanSupplier stateSupplier;
    private Animation toggleAnimation;
    private int colorOn = OpalTheme.first();
    private int colorOff = 0xff3c3c3c;

    private float x, y, width, height;

    public ToggleSwitch(Runnable toggleAction, BooleanSupplier stateSupplier) {
        this.toggleAction = toggleAction;
        this.stateSupplier = stateSupplier;
    }

    public void reset() {
        this.toggleAnimation = null;
    }

    public void setBoxColors(int colorOn, int colorOff) {
        this.colorOn = colorOn;
        this.colorOff = colorOff;
    }

    public void render(float x, float y, float scale) {
        this.x = x;
        this.y = y;
        this.width = 20;
        this.height = 10;

        Skia.save();
        Skia.scale(x, y, width, height, scale);

        float destination = stateSupplier.getAsBoolean() ? 1.0F : 0.0F;
        if (this.toggleAnimation == null) {
            this.toggleAnimation = new Animation(Easing.DECELERATE, 150);
            this.toggleAnimation.setValue(destination);
        } else {
            this.toggleAnimation.run(destination);
        }

        float value = this.toggleAnimation.getValue();
        int trackColor = OpalDraw.interpolate(colorOff, OpalTheme.accent(0), value);
        int trackColor2 = OpalDraw.darker(trackColor, 0.4F);

        OpalDraw.gradientRoundedRect(x, y, width, height, height / 2f, trackColor, trackColor2, 90);
        OpalDraw.gradientRoundedRect(x + 1 + (value * 9.5F), y + 1f, height - 2f, height - 2f, (height - 2f) / 2f,
                0xffffffff, OpalDraw.darker(0xffffffff, 0.1F), 90);

        Skia.restore();
    }

    public void mouseClicked(float boxX, float boxY, float boxWidth, float boxHeight, double mouseX, double mouseY) {
        if (OpalComponent.isHovering(boxX, boxY, boxWidth, boxHeight, mouseX, mouseY)) {
            toggle();
        }
    }

    private void toggle() {
        if (toggleAction != null) {
            toggleAction.run();
        }
    }
}
