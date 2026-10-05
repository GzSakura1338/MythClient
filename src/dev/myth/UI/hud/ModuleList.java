package dev.myth.UI.hud;

import dev.myth.Myth;
import dev.myth.UI.font.FontManager;
import dev.myth.UI.setting.Setting;
import dev.myth.model.Model;
import dev.myth.utils.Keybind;
import imgui.ImGui;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import net.minecraft.client.Minecraft;

/** Mio module/ModuleList + ModuleListCryptoHelper + ModuleListSearchHelper4_2. */
public final class ModuleList extends HudElement {
    public final Setting<Boolean> onlyBound = addSetting(Setting.bool("onlyBound", "OnlyBound", false));
    public final Setting<String> sort = addSetting(Setting.choice("sort", "Sort", "Length", List.of("Alphabet", "Length")));
    private final Map<Model, Entry> entries = new LinkedHashMap<>();
    private List<Entry> ordered = List.of();
    private float width, height;

    public ModuleList() { super("module_list", "ModuleList"); }
    @Override public String getDescription() { return "Draws the enabled module list."; }
    @Override protected void onEnable() { entries.clear(); ordered = List.of(); }

    private static final class Entry {
        final Model model;
        String text;
        float progress, animatedWidth;
        Entry(Model model) { this.model = model; }
        float eased() { return Math.clamp(1 - (float) Math.pow(1 - progress, 4), 0, 1); }
    }
    @Override protected float measureWidth(FontManager fonts) {
        var models = Myth.MODEL_MANAGER.getModels();
        entries.keySet().removeIf(model -> !models.contains(model));
        for (Model model : models) entries.computeIfAbsent(model, Entry::new);
        ordered = new ArrayList<>(entries.values());
        for (Entry entry : ordered) entry.text = entry.model.getInfoString();
        ordered.sort(sort.get().equals("Alphabet") ? Comparator.comparing(entry -> entry.text)
                : Comparator.comparingDouble(entry -> -fonts.width(entry.text)));
        int fps = Minecraft.getInstance().getFps();
        float step = 1f / Math.max(1, fps >> 2);
        float factor = Math.clamp(4 * ImGui.getIO().getDeltaTime(), 0, 1);
        factor = 1 - (float) Math.pow(1 - factor, 4);
        width = height = 0;
        for (Entry entry : ordered) {
            boolean bound = Myth.MODEL_MANAGER.getKeybind(entry.model.getId())
                    .map(key -> key.getBoundKey().getValue() != Keybind.UNBOUND).orElse(false);
            boolean drawn = entry.model.isEnabled() && entry.model.isDrawn() && (!onlyBound.get() || bound);
            entry.progress = Math.clamp(entry.progress + (drawn ? step : -step), 0, 1);
            float textWidth = fonts.width(entry.text);
            entry.animatedWidth = Myth.HUD_MANAGER.smoothWidth.get() && fps > 15
                    ? entry.animatedWidth + (textWidth - entry.animatedWidth) * factor : textWidth;
            if (Math.abs(textWidth - entry.animatedWidth) < .5f) entry.animatedWidth = textWidth;
            if (entry.progress == 0) continue;
            width = Math.max(width, textWidth);
            height += fonts.height() * entry.eased() + 1;
        }
        // Mio gives an empty editor element a 50 x fontHeight placeholder.
        if (width == 0 && Myth.UI_MANAGER.isHudEditorOpen()) { width = 50; height = fonts.height(); }
        return width;
    }
    @Override protected float measureHeight(FontManager fonts) { return height; }

    @Override protected void render(RenderContext context) {
        boolean right = context.x() + context.width() / 2 > context.screenWidth() / 2;
        boolean bottom = context.y() + context.height() / 2 > context.screenHeight() / 2;
        float fontHeight = context.fonts().height();
        float y = bottom ? context.y() + context.height() - fontHeight : context.y();
        for (Entry entry : ordered) {
            float eased = entry.eased();
            float x = getAnchor() == HudAnchor.TOP_CENTER ? context.x() + (context.width() - entry.animatedWidth) / 2
                    : right ? context.x() + context.width() - entry.animatedWidth : context.x();
            x -= entry.animatedWidth * (1 - eased) * (right ? -1 : 1);
            if (entry.progress > 0) {
                int argb = Myth.HUD_MANAGER.colors.colorAt(y, fontHeight);
                float alpha = Math.clamp(entry.progress * 2, .1f, 1) * context.opacity();
                context.fonts().draw(context.draw(), entry.text, x,
                        context.fonts().centeredY(entry.text, y, fontHeight), argb, alpha);
            }
            if (eased != 0) y += (fontHeight * eased + 1) * (bottom ? -1 : 1);
        }
    }
}
