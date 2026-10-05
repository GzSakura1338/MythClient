package today.starain.ui.clickgui.opal.component.property;

import net.minecraft.client.Minecraft;
import today.starain.ui.clickgui.opal.render.OpalDraw;
import today.starain.utils.render.skia.Skia;
import today.starain.value.impl.TextValue;

public final class StringPanel extends PropertyPanel {

    private final TextValue textValue;
    private boolean focused;
    private int cursor;

    public StringPanel(TextValue value) {
        super(value);
        this.textValue = value;
    }

    @Override
    public void render(int mouseX, int mouseY, float delta) {
        setHeight(26);
        super.render(mouseX, mouseY, delta);

        OpalDraw.text(textValue.getName(), x + 5, y + 8.5F, 7, 0xffffffff, false);

        Skia.drawRoundedRect(x + 5, y + 13, width - 10, 10, 2.5F, 0xff191919);
        Skia.drawOutline(x + 5, y + 13, width - 10, 10, 2.5F, 1.5F, OpalDraw.color(0xff505050));

        String value = textValue.get() == null ? "" : textValue.get();
        cursor = Math.min(cursor, value.length());

        OpalDraw.scissor(x + 6, y + 13, width - 12, 10, () -> {
            if (!value.isEmpty()) {
                OpalDraw.text(value, x + 7, y + 20, 7, OpalDraw.opacity(0xffffffff, 0.75F), false);
            }
            if (focused && Minecraft.getInstance().player != null && Minecraft.getInstance().player.tickCount % 20 > 8) {
                float caretX = x + 7.5F + OpalDraw.textWidth(value.substring(0, cursor), 7, false);
                Skia.drawRect(caretX, y + 20 - 5.5F, 0.5F, 7, 0xffffffff);
            }
        });
    }

    @Override
    public void mouseClicked(double mouseX, double mouseY, int button) {
        focused = button == 0 && isHovering(mouseX, mouseY);
        if (focused) {
            cursor = textValue.get() == null ? 0 : textValue.get().length();
        }
    }

    @Override
    public void keyPressed(int key) {
        if (!focused) return;
        String value = textValue.get() == null ? "" : textValue.get();
        switch (key) {
            case 259 -> { // backspace
                if (cursor > 0) {
                    textValue.set(value.substring(0, cursor - 1) + value.substring(cursor));
                    cursor--;
                }
            }
            case 261 -> { // delete
                if (cursor < value.length()) {
                    textValue.set(value.substring(0, cursor) + value.substring(cursor + 1));
                }
            }
            case 263 -> cursor = Math.max(0, cursor - 1); // left
            case 262 -> cursor = Math.min(value.length(), cursor + 1); // right
            case 256, 257 -> focused = false; // escape / enter
        }
    }

    @Override
    public void charTyped(int codepoint) {
        if (!focused || codepoint < 32) return;
        String value = textValue.get() == null ? "" : textValue.get();
        cursor = Math.min(cursor, value.length());
        textValue.set(value.substring(0, cursor) + Character.toString(codepoint) + value.substring(cursor));
        cursor++;
    }
}
