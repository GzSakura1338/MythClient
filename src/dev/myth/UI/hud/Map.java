package dev.myth.UI.hud;

import dev.myth.Myth;
import dev.myth.UI.font.FontManager;
import dev.myth.UI.setting.Setting;

/** Blurred backdrop with a player marker; the HUD collects no world data. */
public final class Map extends HudElement {
    public final Setting<Float> width = addSetting(Setting.decimal("width", "Width", 76, 50, 500));
    public final Setting<Float> height = addSetting(Setting.decimal("height", "Height", 76, 50, 250));
    public final Setting<Boolean> blur = addSetting(Setting.bool("blur", "Blur", true));
    public final Setting<Integer> background = addSetting(Setting.color("background", "Background", 0x320a0a0a, true, false));
    public final Setting<Integer> outline = addSetting(Setting.color("outline", "Outline", 0x640a0a0a, true, false));
    public final Setting<Integer> dot = addSetting(Setting.color("dot", "Dot", 0xffff0000, true, false));
    public final Setting<Float> dotRadius = addSetting(Setting.decimal("dotRadius", "Radius", 2, .5f, 8));
    public final Setting<Float> size = addSetting(Setting.decimal("size", "Size", 1, .5f, 2));
    public Map() { super("map", "Map"); }
    @Override public String getDescription() { return "Blurred backdrop with a player marker."; }
    @Override public boolean wantsBlur() { return blur.get(); }
    @Override protected float measureWidth(FontManager fonts) { return width.get() * size.get(); }
    @Override protected float measureHeight(FontManager fonts) { return height.get() * size.get(); }
    @Override protected void render(RenderContext context) {
        float x = context.x(), y = context.y(), w = context.width(), h = context.height();
        // Mio layers the panel tint over the cached backdrop, one-pixel border included.
        if (blur.get()) Myth.UI_MANAGER.drawHudBlur(context.draw(), x - 1, y - 1, w + 2, h + 2, context.opacity());
        HudDraw.rect(context.draw(), x, y, w, h, background.color(HudDraw.theme()), context.opacity());
        HudDraw.outline(context.draw(), x - 1, y - 1, w + 1, h + 1, outline.color(HudDraw.theme()), context.opacity());
        context.draw().pushClipRect(x, y, x + w, y + h, true);
        context.draw().addCircleFilled(x + w / 2, y + h / 2, dotRadius.get() * size.get(),
                HudDraw.packed(dot.color(HudDraw.theme()), context.opacity()));
        context.draw().popClipRect();
    }
}
