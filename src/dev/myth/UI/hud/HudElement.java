package dev.myth.UI.hud;

import dev.myth.UI.font.FontManager;
import dev.myth.UI.setting.Setting;
import imgui.ImDrawList;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

/** Base for future HUD components; no concrete components are registered. */
public abstract class HudElement {
    public record RenderContext(ImDrawList draw, FontManager fonts, float screenWidth, float screenHeight,
                                float x, float y, float width, float height, float opacity, boolean editing) {}
    private final String id, name;
    private final List<Setting<?>> settings = new ArrayList<>();
    private boolean enabled;
    private HudAnchor anchor = HudAnchor.NONE;
    private float relativeX = .5f, relativeY = .5f;
    private float x, y, width, height;

    protected HudElement(String id, String name) {
        if (!Objects.requireNonNull(id).matches("[a-z0-9_]+") || Objects.requireNonNull(name).isBlank())
            throw new IllegalArgumentException("Invalid HUD element name");
        this.id = id; this.name = name;
    }
    public final String getId() { return id; }
    public final String getName() { return name; }
    public final boolean isEnabled() { return enabled; }
    public final void setEnabled(boolean enabled) { this.enabled = enabled; }
    public final HudAnchor getAnchor() { return anchor; }
    public final float getRelativeX() { return relativeX; }
    public final float getRelativeY() { return relativeY; }
    public final float getX() { return x; }
    public final float getY() { return y; }
    public final float getWidth() { return width; }
    public final float getHeight() { return height; }
    public final List<Setting<?>> getSettings() { return List.copyOf(settings); }
    protected final <T> Setting<T> addSetting(Setting<T> setting) {
        Objects.requireNonNull(setting);
        if (settings.stream().anyMatch(value -> value.getId().equals(setting.getId())))
            throw new IllegalArgumentException("Duplicate HUD setting: " + setting.getId());
        settings.add(setting); return setting;
    }
    public final void setRelativePosition(float x, float y) {
        if (!Float.isFinite(x) || !Float.isFinite(y)) throw new IllegalArgumentException("Invalid HUD position");
        relativeX = Math.clamp(x, 0, 1); relativeY = Math.clamp(y, 0, 1);
    }
    public final void setPosition(float x, float y, float screenWidth, float screenHeight) {
        setRelativePosition(x / Math.max(1, screenWidth), y / Math.max(1, screenHeight));
        layout(x, y, screenWidth, screenHeight);
    }
    public final boolean contains(float mouseX, float mouseY) {
        return enabled && mouseX >= x && mouseX <= x + Math.max(width, 4)
                && mouseY >= y && mouseY <= y + Math.max(height, 4);
    }
    final void anchor(HudAnchor anchor) { this.anchor = Objects.requireNonNull(anchor); }
    final void measure(FontManager fonts) {
        width = enabled ? measureWidth(fonts) : 0; height = enabled ? measureHeight(fonts) : 0;
        if (!Float.isFinite(width) || !Float.isFinite(height) || width < 0 || height < 0)
            throw new IllegalArgumentException("Invalid HUD dimensions: " + id);
    }
    final void layout(float x, float y, float screenWidth, float screenHeight) {
        this.x = Math.clamp(x, 0, Math.max(0, screenWidth - width));
        this.y = Math.clamp(y, 0, Math.max(0, screenHeight - height));
    }
    protected abstract float measureWidth(FontManager fonts);
    protected abstract float measureHeight(FontManager fonts);
    protected abstract void render(RenderContext context);
}
