package today.starain.ui.clickgui.opal.component.property;

import today.starain.ui.clickgui.opal.OpalTheme;
import today.starain.ui.clickgui.opal.render.OpalDraw;
import today.starain.utils.animation.Animation;
import today.starain.utils.animation.Easing;
import today.starain.utils.render.skia.Skia;
import today.starain.value.impl.NumberValue;

public final class NumberPanel extends PropertyPanel {

    private final NumberValue numberValue;

    private boolean dragging;
    private Animation dragAnimation;

    public NumberPanel(NumberValue value) {
        super(value);
        this.numberValue = value;
    }

    static String format(double value) {
        if (value == (int) value) {
            return String.valueOf((int) value);
        }
        return String.format("%.3f", value).replaceAll("0+$", "").replaceAll("\\.$", "");
    }

    private static float clamp01(float value) {
        return Math.max(0f, Math.min(1f, value));
    }

    @Override
    public void render(int mouseX, int mouseY, float delta) {
        setHeight(26);
        super.render(mouseX, mouseY, delta);

        OpalDraw.text(numberValue.getName(), x + 5, y + 8.5F, 7, 0xffffffff, false);

        float sliderWidth = width - 12;
        float sliderHeight = 2.5F;
        float sliderX = x + 6;
        float sliderY = y + 13F;

        if (dragging && mouseX != -1) {
            float percent = clamp01((mouseX - sliderX) / sliderWidth);
            double newValue = numberValue.getMin() + (numberValue.getMax() - numberValue.getMin()) * percent;
            numberValue.setValue(newValue);
        }

        double range = numberValue.getMax() - numberValue.getMin();
        double widthPercent = range == 0 ? 0 : (numberValue.getValue().doubleValue() - numberValue.getMin()) / range;
        float destination = (float) (sliderWidth * widthPercent);

        if (this.dragAnimation == null) {
            this.dragAnimation = new Animation(Easing.LINEAR, 50);
            this.dragAnimation.setValue(destination);
        } else {
            this.dragAnimation.run(destination);
        }

        Skia.drawRoundedRect(sliderX, sliderY, sliderWidth, sliderHeight, sliderHeight / 2f, 0xff373737);

        float dragAnim = dragAnimation.getValue();
        if (dragAnim > 1) {
            int color = OpalTheme.accent(0);
            OpalDraw.gradientRoundedRect(sliderX, sliderY, dragAnim, sliderHeight, sliderHeight / 2f, color, OpalDraw.darker(color, 0.5F), 90);
        }
        OpalDraw.gradientRoundedRect(sliderX + dragAnim - 1, sliderY - 1.3f, 2, 5, 1, 0xffffffff, OpalDraw.darker(0xffffffff, 0.1F), 90);

        String valueString = format(numberValue.getValue().doubleValue());
        OpalDraw.centeredText(valueString, sliderX + dragAnim, y + 22f, 5.5F, OpalDraw.opacity(0xffffffff, 0.8F), false);
    }

    @Override
    public void mouseClicked(double mouseX, double mouseY, int button) {
        if (isHovering(mouseX, mouseY) && button == 0) {
            dragging = true;
        }
    }

    @Override
    public void mouseReleased(double mouseX, double mouseY, int button) {
        if (dragging && button == 0) {
            dragging = false;
        }
    }
}
