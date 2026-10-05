package dev.myth.UI.hud;

import dev.myth.Myth;
import dev.myth.UI.font.FontManager;
import imgui.ImGui;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import net.minecraft.client.Minecraft;

/** Shared Mio text-row animation and docking for effects and metrics. */
abstract class LinesHudElement extends HudElement {
    protected record Line(String id, String text, Integer color) {
        Line(String id, String text) { this(id, text, null); }
    }
    private static final class Entry {
        Line line;
        float progress, width;
        Entry(Line line) { this.line = line; }
        float eased() { return 1 - (float) Math.pow(1 - progress, 4); }
    }
    private final Map<String, Entry> entries = new LinkedHashMap<>();
    private List<Entry> ordered = List.of();
    private float height;
    private final boolean animated;
    protected LinesHudElement(String id, String name, boolean animated) { super(id, name); this.animated = animated; }
    protected abstract List<Line> getLines(FontManager fonts);
    @Override protected void onEnable() { entries.clear(); ordered = List.of(); }
    @Override protected float measureWidth(FontManager fonts) {
        List<Line> lines = getLines(fonts);
        for (Line line : lines) entries.computeIfAbsent(line.id(), ignored -> new Entry(line)).line = line;
        List<String> ids = lines.stream().map(Line::id).toList();
        ordered = new ArrayList<>();
        for (String id : ids) ordered.add(entries.get(id));
        for (var entry : entries.entrySet()) if (!ids.contains(entry.getKey())) ordered.add(entry.getValue());
        int fps = Minecraft.getInstance().getFps();
        float step = 1f / Math.max(1, fps >> 2);
        float factor = 1 - (float) Math.pow(1 - Math.clamp(4 * ImGui.getIO().getDeltaTime(), 0, 1), 4);
        float width = 0; height = 0;
        for (Entry entry : ordered) {
            boolean drawn = ids.contains(entry.line.id());
            entry.progress = animated ? Math.clamp(entry.progress + (drawn ? step : -step), 0, 1) : drawn ? 1 : 0;
            float target = fonts.width(entry.line.text());
            entry.width = animated && Myth.HUD_MANAGER.smoothWidth.get() && fps > 15
                    ? entry.width + (target - entry.width) * factor : target;
            if (Math.abs(target - entry.width) < .5f) entry.width = target;
            if (entry.progress == 0) continue;
            width = Math.max(width, target); height += fonts.height() * entry.eased() + 1;
        }
        entries.entrySet().removeIf(e -> !ids.contains(e.getKey()) && e.getValue().progress == 0);
        if (width == 0 && Myth.UI_MANAGER.isHudEditorOpen()) { width = 50; height = fonts.height(); }
        return width;
    }
    @Override protected float measureHeight(FontManager fonts) { return height; }
    @Override protected void render(RenderContext context) {
        boolean right = context.x() + context.width() / 2 > context.screenWidth() / 2;
        boolean bottom = context.y() + context.height() / 2 > context.screenHeight() / 2;
        float h = context.fonts().height();
        float y = bottom ? context.y() + context.height() - h : context.y();
        for (Entry entry : ordered) {
            if (entry.progress == 0) continue;
            float x = getAnchor() == HudAnchor.TOP_CENTER ? context.x() + (context.width() - entry.width) / 2
                    : right ? context.x() + context.width() - entry.width : context.x();
            if (animated) x -= entry.width * (1 - entry.eased()) * (right ? -1 : 1);
            int color = entry.line.color() == null ? Myth.HUD_MANAGER.colors.colorAt(y, h) : entry.line.color();
            context.fonts().draw(context.draw(), entry.line.text(), x, context.fonts().centeredY(entry.line.text(), y, h),
                    color, context.opacity() * (animated ? Math.clamp(entry.progress * 2, .1f, 1) : 1));
            y += (h * entry.eased() + 1) * (bottom ? -1 : 1);
        }
    }
}
