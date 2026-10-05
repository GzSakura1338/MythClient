package today.starain.ui.clickgui.opal.component.property;

import today.starain.ui.clickgui.opal.OpalTheme;
import today.starain.ui.clickgui.opal.render.OpalDraw;
import today.starain.utils.animation.Animation;
import today.starain.utils.animation.Easing;
import today.starain.utils.render.skia.Skia;
import today.starain.value.impl.NumberValue;

public final class RangeNumberPanel extends PropertyPanel {

    private final NumberValue numberValue;

    private boolean draggingLow, draggingHigh;
    private Animation draggingLowAnimation;
    private Animation draggingHighAnimation;

    public RangeNumberPanel(NumberValue value) {
        super(value);
        this.numberValue = value;
    }

    private static float clamp01(float value) {
        return Math.max(0f, Math.min(1f, value));
    }

    @Override
    public void render(int mouseX, int mouseY, float delta) {
        setHeight(26);
        super.render(mouseX, mouseY, delta);

        OpalDraw.text(numberValue.getName(), x + 5, y + 8.5F, 7, 0xffffffff, false);

        double min = numberValue.getMin();
        double max = numberValue.getMax();
        double range = max - min;
        double lowValue = numberValue.getMinValue().doubleValue();
        double highValue = numberValue.getMaxValue().doubleValue();

        float sliderWidth = width - 12;
        float sliderHeight = 2.5F;
        float sliderX = x + 6;
        float sliderY = y + 13F;

        if (mouseX != -1 && range != 0) {
            float percent = clamp01((mouseX - sliderX) / sliderWidth);
            double newValue = min + range * percent;
            if (draggingLow && newValue <= highValue) {
                numberValue.setMinValue(newValue);
                lowValue = numberValue.getMinValue().doubleValue();
            }
            if (draggingHigh && newValue >= lowValue) {
                numberValue.setMaxValue(newValue);
                highValue = numberValue.getMaxValue().doubleValue();
            }
        }

        double lowPercent = range == 0 ? 0 : (lowValue - min) / range;
        double highPercent = range == 0 ? 0 : (highValue - min) / range;

        float lowDestination = (float) (sliderWidth * lowPercent);
        if (this.draggingLowAnimation == null) {
            this.draggingLowAnimation = new Animation(Easing.LINEAR, 50);
            this.draggingLowAnimation.setValue(lowDestination);
        } else {
            this.draggingLowAnimation.run(lowDestination);
        }

        float highDestination = (float) (sliderWidth * highPercent);
        if (this.draggingHighAnimation == null) {
            this.draggingHighAnimation = new Animation(Easing.LINEAR, 50);
            this.draggingHighAnimation.setValue(highDestination);
        } else {
            this.draggingHighAnimation.run(highDestination);
        }

        Skia.drawRoundedRect(sliderX, sliderY, sliderWidth, sliderHeight, sliderHeight / 2f, 0xff373737);

        float lowAnim = draggingLowAnimation.getValue();
        float highAnim = draggingHighAnimation.getValue();
        if (highAnim > lowAnim) {
            int color = OpalTheme.accent(0);
            OpalDraw.gradientRoundedRect(sliderX + lowAnim, sliderY, highAnim - lowAnim, sliderHeight, sliderHeight / 2f, color, OpalDraw.darker(color, 0.5F), 90);
        }

        OpalDraw.gradientRoundedRect(sliderX + lowAnim - 1, sliderY - 1.3f, 2, 5, 1, 0xffffffff, OpalDraw.darker(0xffffffff, 0.1F), 90);
        OpalDraw.gradientRoundedRect(sliderX + highAnim - 1, sliderY - 1.3f, 2, 5, 1, 0xffffffff, OpalDraw.darker(0xffffffff, 0.1F), 90);

        OpalDraw.centeredText(NumberPanel.format(lowValue), sliderX + lowAnim, y + 22f, 5.5F, OpalDraw.opacity(0xffffffff, 0.8F), false);
        OpalDraw.centeredText(NumberPanel.format(highValue), sliderX + highAnim, y + 22f, 5.5F, OpalDraw.opacity(0xffffffff, 0.8F), false);
    }

    @Override
    public void mouseClicked(double mouseX, double mouseY, int button) {
        if (isHovering(mouseX, mouseY) && button == 0) {
            float sliderWidth = width - 12;
            float sliderX = x + 6;
            double min = numberValue.getMin();
            double range = numberValue.getMax() - min;

            double lowPercent = range == 0 ? 0 : (numberValue.getMinValue().doubleValue() - min) / range;
            double highPercent = range == 0 ? 0 : (numberValue.getMaxValue().doubleValue() - min) / range;

            float lowSliderX = sliderX + (float) (sliderWidth * lowPercent);
            float highSliderX = sliderX + (float) (sliderWidth * highPercent);

            double lowDiff = Math.abs(mouseX - lowSliderX);
            double highDiff = Math.abs(mouseX - highSliderX);
            if (lowDiff == highDiff) {
                if (mouseX < lowSliderX) draggingLow = true;
                else draggingHigh = true;
            } else if (lowDiff < highDiff) {
                draggingLow = true;
            } else {
                draggingHigh = true;
            }
        }
    }

    @Override
    public void mouseReleased(double mouseX, double mouseY, int button) {
        if (button == 0) {
            draggingHigh = draggingLow = false;
        }
    }
}
