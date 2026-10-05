package dev.myth.UI.hud;

import dev.myth.Myth;
import dev.myth.UI.font.FontManager;
import imgui.ImGui;
import net.minecraft.client.Minecraft;

/** Mio CryptoHelper and ModuleListSearchHelper4_2 for a single text row. */
abstract class TextHudElement extends HudElement {
    private String text = "";
    private float progress, animatedWidth, height;

    protected TextHudElement(String id, String name) { super(id, name); }
    protected abstract String getText();
    @Override protected void onEnable() { progress = animatedWidth = 0; }

    @Override protected float measureWidth(FontManager fonts) {
        text = getText();
        int fps = Minecraft.getInstance().getFps();
        progress = Math.min(1, progress + 1f / Math.max(1, fps >> 2));
        float width = fonts.width(text);
        float factor = 1 - (float) Math.pow(1 - Math.clamp(4 * ImGui.getIO().getDeltaTime(), 0, 1), 4);
        animatedWidth = Myth.HUD_MANAGER.smoothWidth.get() && fps > 15
                ? animatedWidth + (width - animatedWidth) * factor : width;
        if (Math.abs(width - animatedWidth) < .5f) animatedWidth = width;
        height = fonts.height() * eased() + 1;
        if (width == 0 && Myth.UI_MANAGER.isHudEditorOpen()) { width = 50; height = fonts.height(); }
        return width;
    }
    private float eased() { return Math.clamp(1 - (float) Math.pow(1 - progress, 4), 0, 1); }
    @Override protected float measureHeight(FontManager fonts) { return height; }

    @Override protected void render(RenderContext context) {
        boolean right = context.x() + context.width() / 2 > context.screenWidth() / 2;
        boolean bottom = context.y() + context.height() / 2 > context.screenHeight() / 2;
        float fontHeight = context.fonts().height();
        float x = getAnchor() == HudAnchor.TOP_CENTER ? context.x() + (context.width() - animatedWidth) / 2
                : right ? context.x() + context.width() - animatedWidth : context.x();
        x -= animatedWidth * (1 - eased()) * (right ? -1 : 1);
        float y = bottom ? context.y() + context.height() - fontHeight : context.y();
        context.fonts().draw(context.draw(), text, x, context.fonts().centeredY(text, y, fontHeight),
                Myth.HUD_MANAGER.colors.colorAt(y, fontHeight), Math.clamp(progress * 2, .1f, 1) * context.opacity());
    }
}
