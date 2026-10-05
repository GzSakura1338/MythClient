package dev.myth.UI.font;

import dev.myth.UI.setting.Setting;
import imgui.ImDrawList;
import java.util.List;

/** Mio Fonts defaults; the custom renderer is selectable without registering a Model. */
public final class FontManager implements AutoCloseable {
    public final Setting<String> font = Setting.text("font", "Font", "Verdana");
    public final Setting<String> style = Setting.choice("style", "Style", "Plain", List.of("Plain", "Bold", "Italic", "BoldItalic"));
    public final Setting<Boolean> antiAlias = Setting.bool("antiAlias", "AntiAlias", false);
    public final Setting<Integer> size = Setting.integer("size", "Size", 16, 12, 30);
    public final Setting<Integer> shift = Setting.integer("shift", "Shift", -2, -4, 4);
    public final Setting<Float> shadow = Setting.decimal("shadow", "Shadow", .7f, .5f, 1);
    public final Setting<Integer> translate = Setting.integer("translate", "Translate", -1, -4, 4);
    private final List<Setting<?>> settings = List.of(font, style, antiAlias, size, shift, shadow, translate);
    private FontAtlas vanilla;
    private AdvanceGlyph custom;
    private String atlasKey = "";
    private boolean enabled;

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
            // Center the visible ink and shadow at the default Shift; retain manual adjustment.
            return top + (height - (helper.textTop(text) + helper.textBottom(text) + shadow.get()) * scale) / 2 + 2 * scale;
        }
        return top + (height - (vanilla.textTop(text) + vanilla.textBottom(text) + 1) * scale) / 2;
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
        if (enabled && custom != null) {
            custom.draw(draw, text, x + offset, y + offset, shadowColor, opacity, scale, true);
            custom.draw(draw, text, x, y, argb, opacity, scale);
        } else {
            // Mio submits integer positions to Minecraft's bitmap renderer.
            x = (float) Math.floor(x); y = (float) Math.floor(y);
            vanilla.draw(draw, text, x + offset, y + offset, shadowColor, opacity, scale);
            vanilla.draw(draw, text, x, y, argb, opacity, scale);
        }
    }

    @Override public void close() {
        if (vanilla != null) vanilla.close();
        if (custom != null) custom.close();
        vanilla = null; custom = null; atlasKey = "";
    }
}
