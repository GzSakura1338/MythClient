package today.starain.ui.clickgui.opal.component.property;

import today.starain.ui.clickgui.opal.render.OpalDraw;
import today.starain.utils.animation.Animation;
import today.starain.utils.animation.Easing;
import today.starain.utils.render.skia.Skia;
import today.starain.value.impl.ModeValue;

public final class ModePanel extends PropertyPanel {

    private final ModeValue modeValue;
    private final Animation expandAnimation = new Animation(Easing.DECELERATE, 125);
    private boolean expanded;

    public ModePanel(ModeValue value) {
        super(value);
        this.modeValue = value;
    }

    @Override
    public void render(int mouseX, int mouseY, float delta) {
        super.render(mouseX, mouseY, delta);
        expandAnimation.run(expanded ? 1 : 0);

        OpalDraw.text(modeValue.getName(), x + 5, y + 9.5F, 7, 0xffffffff, false);

        float padding = 2;
        float rectX = x + 3;
        float rectY = y + padding + 11.5F;
        float rectWidth = width - 5 - padding;
        float boxHeight = height - padding - (32 - DEFAULT_HEIGHT);

        Skia.drawRoundedRect(rectX, rectY, rectWidth, boxHeight, 4, OpalDraw.color(OpalDraw.opacity(0xff000000, 0.25F)));
        OpalDraw.text(modeValue.get(), rectX + 4, rectY + 10, 7, 0xffffffff, true);

        float iconSize = 9;
        float chevronX = rectX + rectWidth - 12 + OpalDraw.iconWidth("\ue5cf", iconSize) / 2f;
        float chevronY = rectY + 2.5F + iconSize / 2f;
        OpalDraw.rotate(chevronX, chevronY, expandAnimation.getValue() * 180,
                () -> OpalDraw.centeredIcon("\ue5cf", chevronX, chevronY, iconSize, 0xffffffff));

        OpalDraw.scissor(rectX, rectY, rectWidth, boxHeight, () -> {
            int addedHeight = 0;
            if (expandAnimation.getValue() > 0) {
                for (String mode : modeValue.getModes()) {
                    if (mode == null || mode.equalsIgnoreCase(modeValue.get())) continue;
                    OpalDraw.text(mode, rectX + 4, rectY + 9.5F + 13 + addedHeight, 7, 0xffffffff, false);
                    addedHeight += 13;
                }
            }
            setHeight(32 + addedHeight * expandAnimation.getValue());
        });
    }

    @Override
    public void mouseClicked(double mouseX, double mouseY, int button) {
        if (isHovering(x, y, width, 32, mouseX, mouseY) && button == 1) {
            expanded = !expanded;
            return;
        }

        if (expanded) {
            float padding = 2;
            float rectX = x + 3;
            float rectY = y + padding + 11.5F;
            float rectWidth = width - 8 - padding - 3f;

            int addedHeight = 0;
            for (String mode : modeValue.getModes()) {
                if (mode == null || mode.equalsIgnoreCase(modeValue.get())) continue;
                if (isHovering(rectX, rectY + 13 + addedHeight, rectWidth, 13, mouseX, mouseY)) {
                    modeValue.set(mode);
                    expanded = false;
                    return;
                }
                addedHeight += 13;
            }
        }
    }
}
