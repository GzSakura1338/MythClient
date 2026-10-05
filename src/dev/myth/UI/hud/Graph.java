package dev.myth.UI.hud;

import dev.myth.Myth;
import dev.myth.UI.font.FontManager;
import dev.myth.UI.setting.Setting;
import java.util.ArrayDeque;

public final class Graph extends HudElement {
    public final Setting<Float> ceil = addSetting(Setting.decimal("ceil", "Ceil", 26, 10, 100));
    public final Setting<Integer> height = addSetting(Setting.integer("height", "Height", 30, 10, 40));
    public final Setting<Integer> width = addSetting(Setting.integer("width", "Width", 100, 50, 150));
    private final ArrayDeque<Double> samples = new ArrayDeque<>();
    public Graph() { super("graph", "Graph"); }
    @Override public String getDescription() { return "Plots the Mio speed history with fading ends."; }
    @Override protected void onEnable() { samples.clear(); for (int i = 0; i < 150; i++) samples.add(0d); }
    @Override protected void onTick() { samples.add(Myth.HUD_MANAGER.data.speedKmh()); while (samples.size() > width.get()) samples.removeFirst(); }
    @Override protected float measureWidth(FontManager fonts) { return width.get(); }
    @Override protected float measureHeight(FontManager fonts) { return height.get(); }
    @Override protected void render(RenderContext context) {
        int index = 0; float previousY = 0;
        for (double value : samples) {
            float y = context.y() + 1 + (height.get() - 2) * (1 - (float) Math.min(value, ceil.get()) / ceil.get());
            if (index > 0) {
                float fade = Math.clamp(Math.min(index / 10f, (width.get() - index) / 10f), 0, 1);
                int color = HudDraw.packed(Myth.HUD_MANAGER.colors.colorAt(y, context.fonts().height()), context.opacity() * fade);
                context.draw().addLine(context.x() + index - 1, previousY, context.x() + index, y, color);
            }
            previousY = y; index++;
        }
    }
}
