package today.starain.ui.clickgui.opal.component.property;

import today.starain.ui.clickgui.opal.render.OpalDraw;
import today.starain.utils.animation.Animation;
import today.starain.utils.animation.Easing;
import today.starain.utils.render.skia.Skia;
import today.starain.value.impl.ColorValue;

import java.awt.*;

public final class ColorPanel extends PropertyPanel {

    private final ColorValue colorValue;
    private final Animation expandAnimation = new Animation(Easing.DECELERATE, 125);
    private final float[] hsb = new float[3];
    private boolean expanded;
    private DragType dragType = DragType.NONE;
    private int alpha = 255;

    public ColorPanel(ColorValue value) {
        super(value);
        this.colorValue = value;
        syncFromValue();
    }

    private static float clamp01(float value) {
        return Math.max(0f, Math.min(1f, value));
    }

    private void syncFromValue() {
        Color color = colorValue.get();
        if (color == null) color = Color.WHITE;
        Color.RGBtoHSB(color.getRed(), color.getGreen(), color.getBlue(), hsb);
        alpha = color.getAlpha();
    }

    private void applyToValue() {
        Color rgb = Color.getHSBColor(hsb[0], hsb[1], hsb[2]);
        colorValue.set(new Color(rgb.getRed(), rgb.getGreen(), rgb.getBlue(), alpha));
    }

    @Override
    public void render(int mouseX, int mouseY, float delta) {
        super.render(mouseX, mouseY, delta);
        expandAnimation.run(expanded ? 1 : 0);

        if (dragType == DragType.NONE) {
            syncFromValue();
        }

        OpalDraw.text(colorValue.getName(), x + 5, y + 10.5F, 7, 0xffffffff, false);
        Skia.drawRoundedRect(x + width - 22, y + 3.5F, 18, 10, 3, colorValue.get() == null ? Color.WHITE : colorValue.get());

        float xPos = x + 5;
        float yPos = y + DEFAULT_HEIGHT;
        float w = 65;
        float h = 50;

        OpalDraw.scissor(x, y, width, height, () -> {
            if (expandAnimation.getValue() > 0) {
                if (dragType == DragType.PICKER) {
                    hsb[1] = clamp01((mouseX - xPos) / w);
                    hsb[2] = clamp01(1 - (mouseY - yPos) / h);
                    applyToValue();
                } else if (dragType == DragType.HUE) {
                    hsb[0] = clamp01((mouseY - yPos) / h);
                    applyToValue();
                }

                int base = Color.HSBtoRGB(hsb[0], 1, 1) | 0xFF000000;
                Skia.drawRect(xPos, yPos, w, h, base);
                OpalDraw.gradientRect(xPos, yPos, w, h, 0xffffffff, 0x00ffffff, 0);
                OpalDraw.gradientRect(xPos, yPos, w, h, 0x00000000, 0xff000000, 90);

                OpalDraw.rainbowRect(xPos + w + 5, yPos, 8, h);
            }

            setHeight(DEFAULT_HEIGHT + (h * expandAnimation.getValue()));
        });
    }

    @Override
    public void mouseClicked(double mouseX, double mouseY, int button) {
        if (isHovering(x, y, width, DEFAULT_HEIGHT, mouseX, mouseY) && button == 1) {
            expanded = !expanded;
            return;
        }

        if (button == 0 && expanded) {
            float xPos = x + 5;
            float yPos = y + DEFAULT_HEIGHT;
            float w = 65;
            float h = 50;
            if (isHovering(xPos, yPos, w, h, mouseX, mouseY)) {
                dragType = DragType.PICKER;
            } else if (isHovering(xPos + w + 5, yPos, 8, h, mouseX, mouseY)) {
                dragType = DragType.HUE;
            }
        }
    }

    @Override
    public void mouseReleased(double mouseX, double mouseY, int button) {
        dragType = DragType.NONE;
    }

    private enum DragType {PICKER, HUE, NONE}
}
