package dev.myth.UI.font;

import dev.myth.UI.setting.Setting;
import imgui.ImDrawList;
import java.util.List;

/** Saved font preferences; the custom renderer is selectable without registering a Model. */
public final class FontManager implements AutoCloseable {
    public final Setting<String> font = Setting.text("font", "Font", "Verdana");
    public final Setting<String> style = Setting.choice("style", "Style", "Plain", List.of("Plain", "Bold", "Italic", "BoldItalic"));
    public final Setting<Boolean> antiAlias = Setting.bool("antiAlias", "AntiAlias", true);
    public final Setting<Integer> size = Setting.integer("size", "Size", 17, 12, 30);
    public final Setting<Integer> shift = Setting.integer("shift", "Shift", -2, -4, 4);
    public final Setting<Float> shadow = Setting.decimal("shadow", "Shadow", .7f, .5f, 1);
    public final Setting<Integer> translate = Setting.integer("translate", "Translate", -1, -4, 4);
    private final List<Setting<?>> settings = List.of(font, style, antiAlias, size, shift, shadow, translate);
    private FontAtlas vanilla;
    private AdvanceGlyph custom;
    private String atlasKey = "";
    private boolean enabled = true;

    public boolean isEnabled() { return enabled; }
    public void setEnabled(boolean enabled) { this.enabled = enabled; }
    public List<Setting<?>> getSettings() { return settings; }

    public void prepare() {
        if (vanilla == null) vanilla = FontAtlas.minecraft();
        if (!enabled) return;
        String key = font.get() + "/" + style.get() + "/" + size.get() + "/" + antiAlias.get();
        if (!key.equals(atlasKey)) {
            // Mio waits until the size slider is released before rebuilding glyphs.
            if (custom != null && imgui.ImGui.isMouseDown(0)) return;
            AdvanceGlyph next = AdvanceGlyph.create(font.get(), size.get(), style.getChoices().indexOf(style.get()), antiAlias.get());
            if (custom != null) custom.close();
            custom = next;
            atlasKey = key;
        }
    }

    public float height() { return enabled && custom != null ? custom.get2771() - 1 : 9; }
    public float centeredY(String text, float top, float height) { return centeredY(text, top, height, 1); }
    public float centeredY(String text, float top, float height, float scale) {
        if (enabled && custom != null) {
            FontHelper helper = custom.fontHelper;
            // Center the foreground glyphs; their drop shadow must not move the
            // text upward. Preserve the existing manual Shift adjustment.
            return top + (height - (helper.textTop(text) + helper.textBottom(text)) * scale) / 2 + 2 * scale;
        }
        return top + (height - (vanilla.textTop(text) + vanilla.textBottom(text)) * scale) / 2;
    }
    public float width(String text) { return enabled && custom != null ? custom.get2772(text) : vanilla.width(text); }
    public void draw(ImDrawList draw, String text, float x, float y, int argb, float opacity) {
        draw(draw, text, x, y, argb, opacity, 1);
    }
    public void draw(ImDrawList draw, String text, float x, float y, int argb, float opacity, float scale) {
        if (enabled) {
            x += translate.get() * scale; y += shift.get() * scale;
        }
        float offset = (enabled ? shadow.get() : 1) * scale;
        int shadowColor = (argb & 0xff000000) | ((argb & 0xfcfcfc) >> 2);
        boolean customFont = enabled && custom != null;
        // Keep foreground ink centered on framebuffer pixels. Nearest sampling
        // can change its bounds at fractional scales, independently of shadows.
        float pixelScaleY = imgui.ImGui.getIO().getDisplayFramebufferScaleY();
        float inkTop = (customFont ? custom.fontHelper.textTop(text) : vanilla.textTop(text)) * scale * pixelScaleY;
        float inkBottom = (customFont ? custom.fontHelper.textBottom(text) : vanilla.textBottom(text)) * scale * pixelScaleY;
        float rasterCenter = (Math.round(inkTop) + Math.round(inkBottom)) / 2f;
        float correction = customFont && antiAlias.get() ? 0 : (inkTop + inkBottom) / 2 - rasterCenter;
        y = Math.round(y * pixelScaleY + correction) / pixelScaleY;
        if (customFont) {
            custom.draw(draw, text, x + offset, y + offset, shadowColor, opacity, scale, true);
            custom.draw(draw, text, x, y, argb, opacity, scale);
        } else {
            x = (float) Math.floor(x);
            vanilla.draw(draw, text, x + offset, y + offset, shadowColor, opacity, scale, true);
            vanilla.draw(draw, text, x, y, argb, opacity, scale);
        }
    }

    @Override public void close() {
        if (vanilla != null) vanilla.close();
        if (custom != null) custom.close();
        vanilla = null; custom = null; atlasKey = "";
    }
}
