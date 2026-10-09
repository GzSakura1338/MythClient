package dev.myth.UI;

import com.mojang.blaze3d.platform.InputConstants;
import dev.myth.Myth;
import dev.myth.UI.font.FontManager;
import dev.myth.UI.setting.Setting;
import dev.myth.UI.hud.HudElement;
import dev.myth.model.Category;
import dev.myth.model.Model;
import dev.myth.utils.Keybind;
import imgui.ImDrawList;
import imgui.ImGui;
import imgui.flag.ImGuiMouseCursor;
import imgui.flag.ImGuiWindowFlags;
import java.awt.Color;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ThreadLocalRandom;
import net.minecraft.client.Minecraft;
import net.minecraft.client.input.KeyEvent;
import org.lwjgl.glfw.GLFW;

/** Mio's floating category frames and controls, drawn by ImGui without its stock window chrome. */
public final class ClickGui {
    static final int PANEL_GAP = 5;

    public static final class Panel {
        public final Category category;
        public float x;
        public float y = 5;
        public boolean expanded = true;
        private final Animation opening = new Animation(1);
        private float height = 15;
        Panel(Category category, float x) { this.category = category; this.x = x; }
    }

    private static final class Row {
        final String id, name, description;
        final Model model;
        final HudElement hud;
        final List<Setting<?>> settings;
        boolean expanded;
        final Animation opening = new Animation(0), enabled = new Animation(0), bounce = new Animation(0), keyBounce = new Animation(0);
        float height;
        long flash;
        Row(String id, String name, String description, Model model, List<Setting<?>> settings) {
            this(id, name, description, model, null, settings);
        }
        Row(String id, String name, String description, Model model, HudElement hud, List<Setting<?>> settings) {
            this.id = id; this.name = name; this.description = description; this.model = model; this.hud = hud; this.settings = settings;
        }
    }
    private static final class Control {
        final Animation fill = new Animation(0), bounce = new Animation(0);
        final Animation saturation = new Animation(0), brightness = new Animation(0), hue = new Animation(0), alpha = new Animation(0), pointerSize = new Animation(0);
        boolean open;
        long hoveredSince;
        long copied, pasted, invalid;
        float[] hsv;
        int lastColor;
        final Setting<String> filter = Setting.text("filter", "Search", "");
        int page;
    }
    private static final class SoundPanel {
        final String name;
        final List<UiSounds.Entry> entries = new ArrayList<>();
        float x, y = 5;
        boolean expanded = true;
        SoundPanel(String name, float x) { this.name = name; this.x = x; }
    }
    private static final class Snow {
        final float speed = Math.clamp(ThreadLocalRandom.current().nextFloat(), .075f, .175f);
        final float scale = ThreadLocalRandom.current().nextFloat(.01f, 1.15f);
        final float x = ThreadLocalRandom.current().nextFloat();
        final float drift = ThreadLocalRandom.current().nextFloat(-.035f, .035f);
        float progress = ThreadLocalRandom.current().nextFloat(-.4f, 0);
    }

    private final UiManager manager;
    private final UiOptions options;
    private final FontManager fonts;
    private final List<Panel> panels = new ArrayList<>();
    private final Map<String, Row> rows = new LinkedHashMap<>();
    private final Animation hudPanelOpening = new Animation(1);
    private final Map<Setting<?>, Control> controls = new HashMap<>();
    private final List<Snow> snow = new ArrayList<>();
    private Panel dragging;
    private float dragX, dragY;
    private Panel owner;
    private ImDrawList draw;
    private float mx, my, screenWidth, screenHeight, opacity = 1, delta;
    private float laidOutWidth, laidOutHeight;
    private float clipTop, clipBottom;
    private boolean searchFocused;
    private String search = "", suggestion = "";
    private Setting<?> editing;
    private String editBuffer = "";
    private int editCursor;
    private Keybind binding;
    private Keybind hoveredBinding;
    private Setting<?> slider;
    private Setting<Integer> colorDragging;
    private int colorPart;
    private String soundPicker;
    private Setting<String> soundSetting;
    private final List<SoundPanel> soundPanels = new ArrayList<>();
    private SoundPanel draggingSound;
    private String hoveredNow = "", tooltip = "";
    private final Set<String> previousHover = new HashSet<>(), currentHover = new HashSet<>();
    private final List<Runnable> inputActions = new ArrayList<>();
    private final boolean[] clickUsed = new boolean[3];
    private long tooltipStarted;
    private String tooltipOwner = "";

    ClickGui(UiManager manager, UiOptions options, FontManager fonts) {
        this.manager = manager; this.options = options; this.fonts = fonts;
        float x = 10;
        for (Category category : List.of(Category.COMBAT, Category.MISC, Category.RENDER, Category.PLAYER, Category.CLIENT)) {
            panels.add(new Panel(category, x)); x += options.width() + PANEL_GAP;
        }
        rows.put("@ui", new Row("@ui", "UI", "Displays the client's click gui.", null, options.getSettings()));
        rows.put("@fonts", new Row("@fonts", "Fonts", "Manages the client's font renderer.", null, fonts.getSettings()));
        rows.put("@hud", new Row("@hud", "HUD", "Manages HUD elements and their editor.", null, Myth.HUD_MANAGER.getSettings()));
        for (int i = 0; i < 25; i++) snow.add(new Snow());
    }

    public List<Panel> getPanels() { return List.copyOf(panels); }
    public void applyPanelOrder(List<UiManager.CategoryName> order) {
        panels.sort(java.util.Comparator.comparingInt(panel -> {
            int index = order.indexOf(UiManager.CategoryName.valueOf(panel.category.name()));
            return index < 0 ? order.size() : index;
        }));
    }
    public Map<String, Boolean> getExpandedRows() {
        Map<String, Boolean> result = new LinkedHashMap<>(); rows.forEach((id, row) -> result.put(id, row.expanded)); return result;
    }
    public void applyExpandedRows(Map<String, Boolean> states) {
        refreshModels(); states.forEach((id, state) -> { if (rows.containsKey(id)) rows.get(id).expanded = state; });
    }
    public void resetLayout() {
        panels.sort(java.util.Comparator.comparingInt(panel -> categoryOrder(panel.category)));
        float x = 10;
        for (Panel panel : panels) { panel.x = x; x += options.width() + PANEL_GAP; }
        alignPanels();
    }
    private static int categoryOrder(Category category) {
        return switch (category) {
            case COMBAT -> 0; case MISC -> 1; case RENDER -> 2; case PLAYER -> 3; case CLIENT -> 4;
        };
    }
    /** Scrolling deliberately parks panels off-screen, so only a viewport change pulls them back:
     *  a panel dragged to the far side of a large window is otherwise lost when the window shrinks. */
    private void reclampPanels() {
        if (screenWidth == laidOutWidth && screenHeight == laidOutHeight) return;
        laidOutWidth = screenWidth; laidOutHeight = screenHeight;
        float maxX = Math.max(0, screenWidth - options.width()), maxY = Math.max(0, screenHeight - 14);
        for (Panel panel : panels) {
            panel.x = Math.clamp(panel.x, 0, maxX);
            // A panel taller than the viewport keeps its scrolled offset, but not past its own bottom.
            panel.y = Math.clamp(panel.y, Math.min(0, screenHeight - panel.height), maxY);
        }
        for (SoundPanel panel : soundPanels) {
            panel.x = Math.clamp(panel.x, 0, maxX); panel.y = Math.clamp(panel.y, Math.min(0, maxY), maxY);
        }
    }
    private void alignPanels() {
        if (options.position.get().equals("Custom")) return;
        float totalWidth = panels.size() * options.width() + (panels.size() - 1) * PANEL_GAP;
        float x = Math.max(0, switch (options.position.get()) {
            case "Center" -> (screenWidth - totalWidth) / 2;
            case "Right" -> screenWidth - totalWidth - 10;
            default -> 10;
        });
        for (Panel panel : panels.stream().sorted(java.util.Comparator.comparingInt(p -> categoryOrder(p.category))).toList()) {
            panel.x = x; panel.y = 5;
            x += options.width() + PANEL_GAP;
        }
    }
    public void opened() {
        search = ""; suggestion = ""; searchFocused = false; editing = null; binding = null;
        dragging = null; slider = null; colorDragging = null; soundPicker = null;
        previousHover.clear(); currentHover.clear(); tooltipStarted = 0;
        for (Row row : rows.values()) {
            row.enabled.snap(isEnabled(row) ? 1 : 0);
            row.opening.snap(row.expanded ? settingsHeight(row) : 0);
        }
    }
    public void removed() { editing = null; binding = null; dragging = null; slider = null; colorDragging = null; previousHover.clear(); currentHover.clear(); inputActions.clear(); }

    private void refreshModels() {
        for (Model model : Myth.MODEL_MANAGER.getModels()) rows.computeIfAbsent(model.getId(), id ->
                new Row(id, model.getName(), model.getDescription(), model, model.getSettings()));
        for (HudElement element : Myth.HUD_MANAGER.getElements()) rows.computeIfAbsent("@hud/" + element.getId(), id ->
                new Row(id, element.getName(), element.getDescription(), null, element, element.getSettings()));
    }
    private List<Row> categoryRows(Category category) {
        return rows.values().stream().filter(row -> row.hud == null && (row.model == null ? category == Category.CLIENT : row.model.getCategory() == category)
                && matches(row)).toList();
    }
    private boolean matches(Row row) {
        if (search.isEmpty()) return true;
        String lower = search.toLowerCase(Locale.ROOT);
        return row.name.toLowerCase(Locale.ROOT).contains(lower) || row.id.toLowerCase(Locale.ROOT).contains(lower);
    }
    private boolean isEnabled(Row row) {
        if (row.hud != null) return row.hud.isEnabled();
        return row.model != null ? row.model.isEnabled() : row.id.equals("@ui") || (row.id.equals("@hud") ? Myth.HUD_MANAGER.isEnabled() : fonts.isEnabled());
    }
    private float rowHeight() { return 2 + fonts.height() + options.buttonHeight.get(); }
    private float controlHeight(Setting<?> setting) {
        if (!setting.isVisible()) return 0;
        Control state = controls.computeIfAbsent(setting, ignored -> new Control());
        if (!state.open) return rowHeight();
        return switch (setting.getKind()) {
            case ENUM -> rowHeight() * (1 + setting.getChoices().size()) + 1;
            case SET -> rowHeight() * (3 + Math.min(8, selectionMatches(setting, state).size()));
            case COLOR -> (int) (options.width() + 7 + rowHeight() * (setting.canSync() ? 4 : 3) + (setting.hasAlpha() ? 10 : 2) + .5f);
            default -> rowHeight();
        };
    }
    private float settingsHeight(Row row) {
        float height = row.hud == null ? rowHeight() : 0; // HUD elements have no binding row.
        for (Setting<?> setting : row.settings) height += controlHeight(setting);
        return height;
    }
    private List<String> selectionMatches(Setting<?> setting, Control state) {
        String query = (editing == state.filter ? editBuffer : state.filter.get()).toLowerCase(Locale.ROOT);
        return setting.getEntries().stream().filter(entry -> entry.toLowerCase(Locale.ROOT).contains(query)).toList();
    }
    private List<Row> hudRows() {
        return rows.values().stream().filter(row -> row.hud != null).sorted(java.util.Comparator.comparing(row -> row.name)).toList();
    }

    public void render() {
        refreshModels(); fonts.prepare();
        var io = ImGui.getIO(); delta = io.getDeltaTime();
        screenWidth = io.getDisplaySizeX(); screenHeight = io.getDisplaySizeY();
        reclampPanels(); alignPanels();
        mx = io.getMousePosX(); my = io.getMousePosY(); opacity = manager.opacity();
        draw = ImGui.getBackgroundDrawList();
        renderSnow();
        hoveredNow = ""; tooltip = "";
        currentHover.clear(); inputActions.clear(); java.util.Arrays.fill(clickUsed, false);
        hoveredBinding = null;
        if (soundPicker != null) {
            ImGui.setNextWindowPos(0, 0); ImGui.setNextWindowSize(screenWidth, screenHeight);
            ImGui.begin("##mio-sounds", ImGuiWindowFlags.NoDecoration | ImGuiWindowFlags.NoBackground | ImGuiWindowFlags.NoSavedSettings | ImGuiWindowFlags.NoMove | ImGuiWindowFlags.NoNav);
            renderSoundPicker();
            ImGui.setMouseCursor(hoveredNow.isEmpty() ? ImGuiMouseCursor.Arrow : ImGuiMouseCursor.Hand);
            ImGui.end(); finishInput(); return;
        }
        for (Panel panel : panels) {
            float body = 1;
            for (Row row : categoryRows(panel.category)) {
                row.height = rowHeight() + row.opening.update(row.expanded ? settingsHeight(row) : 0, options.animSpeed.get() * 2, delta, true);
                body += row.height;
            }
            panel.height = 14 + body * panel.opening.update(panel.expanded ? 1 : 0, options.animSpeed.get() * 2, delta, true);
        }
        owner = null;
        for (int i = panels.size() - 1; i >= 0; i--) {
            Panel panel = panels.get(i);
            if (inside(panel.x, panel.y, options.width(), panel.height)) { owner = panel; break; }
        }
        if (dragging != null) {
            if (ImGui.isMouseDown(0)) {
                dragging.x = Math.clamp(mx - dragX, 0, Math.max(0, screenWidth - options.width()));
                dragging.y = Math.clamp(my - dragY, 0, Math.max(0, screenHeight - dragging.height));
                owner = dragging;
            } else dragging = null;
        }
        if (!ImGui.isMouseDown(0)) { slider = null; colorDragging = null; }
        ImGui.setNextWindowPos(0, 0); ImGui.setNextWindowSize(screenWidth, screenHeight);
        ImGui.begin("##mio-canvas", ImGuiWindowFlags.NoDecoration | ImGuiWindowFlags.NoBackground | ImGuiWindowFlags.NoSavedSettings
                | ImGuiWindowFlags.NoMove | ImGuiWindowFlags.NoNav | ImGuiWindowFlags.NoBringToFrontOnFocus);
        for (Panel panel : panels) manager.drawPanelBlur(draw, panel.x, panel.y, options.width(),
                categoryRows(panel.category).isEmpty() || panel.opening.value() == 0 ? 14 : panel.height + .5f);
        for (Panel panel : panels) renderPanel(panel);
        renderSearch();
        renderTooltip();
        ImGui.setMouseCursor(editing != null || searchFocused || binding != null ? ImGuiMouseCursor.TextInput
                : hoveredNow.isEmpty() ? ImGuiMouseCursor.Arrow : ImGuiMouseCursor.Hand);
        ImGui.end();
        finishInput();
    }

    /** Reuse the Mio row/setting controls in the HUD editor category panel. */
    public float prepareHudPanel() {
        refreshModels();
        float body = Myth.HUD_MANAGER.getElements().isEmpty() ? 0 : 1;
        float dt = ImGui.getIO().getDeltaTime();
        for (Row row : rows.values()) if (row.hud != null) {
            row.height = rowHeight() + row.opening.update(row.expanded ? settingsHeight(row) : 0, options.animSpeed.get() * 2, dt, true);
            body += row.height;
        }
        return 14 + body * hudPanelOpening.update(Myth.HUD_MANAGER.isPanelExpanded() ? 1 : 0,
                options.animSpeed.get() * 2, Math.min(dt, 1f / 30), true);
    }
    public void renderHudRows(float x, float y, float width, float height, float alpha, boolean active) {
        delta = ImGui.getIO().getDeltaTime(); screenWidth = ImGui.getIO().getDisplaySizeX(); screenHeight = ImGui.getIO().getDisplaySizeY();
        mx = ImGui.getMousePosX(); my = ImGui.getMousePosY(); opacity = alpha; draw = ImGui.getBackgroundDrawList();
        hoveredNow = ""; tooltip = ""; hoveredBinding = null;
        currentHover.clear(); inputActions.clear(); java.util.Arrays.fill(clickUsed, false);
        if (!ImGui.isMouseDown(0)) { slider = null; colorDragging = null; }
        clipTop = Math.max(0, y + 14); clipBottom = Math.min(screenHeight, y + height);
        ImGui.setNextWindowPos(0, 0); ImGui.setNextWindowSize(screenWidth, screenHeight);
        ImGui.begin("##mio-hud-controls", ImGuiWindowFlags.NoDecoration | ImGuiWindowFlags.NoBackground | ImGuiWindowFlags.NoSavedSettings
                | ImGuiWindowFlags.NoMove | ImGuiWindowFlags.NoNav | ImGuiWindowFlags.NoBringToFrontOnFocus);
        if (height > 14) {
            rect(x, y + 14, x + width, y + height + .5f, options.color(options.bgColor));
            draw.pushClipRect(x, clipTop, x + width, Math.max(clipTop, clipBottom), true);
            float offset = y + 15;
            for (Row row : hudRows()) {
                renderRow(row, x, offset, width, active && Myth.HUD_MANAGER.isPanelExpanded()); offset += row.height;
            }
            draw.popClipRect();
        }
        renderTooltip(); ImGui.end(); finishInput();
    }

    private void renderPanel(Panel panel) {
        float x = panel.x, y = panel.y, width = options.width();
        boolean active = owner == panel && soundPicker == null && !manager.isClosing();
        boolean header = active && inside(x, y, width, 14);
        click(header, 0, () -> {
            options.position.set("Custom");
            dragging = panel; dragX = mx - x; dragY = my - y;
            resetSearch(); editing = null;
            panels.remove(panel); panels.add(panel);
        });
        click(header, 1, () -> panel.expanded = !panel.expanded);
        // Match the body backdrop before blending a translucent theme color.
        rect(x, y, x + width, y + 14, options.color(options.bgColor));
        rect(x, y, x + width, y + 14, options.themeColor());
        String title = panel.category.getId().substring(0, 1).toUpperCase(Locale.ROOT) + panel.category.getId().substring(1);
        manager.getIcons().drawTitle(draw, panel.category.getId(), title, fonts, x, y, width, opacity);
        var rows = categoryRows(panel.category);
        if (!rows.isEmpty() && panel.opening.value() > 0) {
            rect(x, y + 14, x + width, y + panel.height + .5f, options.color(options.bgColor));
            clipTop = Math.max(0, y + 14); clipBottom = Math.min(screenHeight, y + panel.height);
            draw.pushClipRect(x, clipTop, x + width, clipBottom, true);
            float offset = y + 15;
            for (Row row : rows) { renderRow(row, x, offset, width, active && panel.expanded); offset += row.height; }
            draw.popClipRect();
            if (options.windowShadow.get()) {
                float shadow = options.shadowSize.get(); int color = options.color(options.shadowColor);
                rect(x + shadow, y + panel.height, x + width, y + panel.height + shadow, color);
                rect(x + width, y + shadow, x + width + shadow, y + panel.height + shadow, color);
            }
        }
        if (options.line.get()) panelOutline(x - 1, y - 1, x + width, y + (rows.isEmpty() || panel.opening.value() == 0 ? 14 : panel.height + .5f));
    }

    private void renderRow(Row row, float x, float y, float width, boolean active) {
        float height = rowHeight(); boolean hovered = hit(x, y, width, height, active);
        hover(row.id, hovered, row.description);
        float bounce = row.bounce.update(hovered && options.bounce.get() ? 1 : 0, 4, delta, true);
        float fill = row.enabled.update(isEnabled(row) ? 1 : 0, options.animSpeed.get() * 2, delta, true);
        // Mio draws the enabled and disabled portions side by side, without
        // blending an enabled rectangle over a second button background.
        if (fill > 0) {
            float split = x + (width - 1) * fill;
            rect(split + 1, y + .5f, x + width - 1, y + height - .5f, options.color(options.bgButton));
            rect(x + 1, y + .5f, split, y + height - .5f, options.color(options.bgEnabled));
        } else rect(x + 1, y + .5f, x + width - 1, y + height - .5f, options.color(options.bgButton));
        if (row.flash > System.currentTimeMillis()) {
            float flash = Math.abs((row.flash - System.currentTimeMillis()) % 300 - 150) / 150f;
            rect(x + 1, y + .5f, x + width - 1, y + height - .5f, ((int) (flash * 150) << 24) | 0xff0000);
        }
        activate(hovered, 0, () -> {
            resetSearch(); editing = null;
            if (row.hud != null) row.hud.setEnabled(!row.hud.isEnabled());
            else if (row.model != null) manager.toggle(row.model);
            else if (row.id.equals("@ui")) manager.requestClose();
            else if (row.id.equals("@hud")) Myth.HUD_MANAGER.setEnabled(!Myth.HUD_MANAGER.isEnabled());
            else fonts.setEnabled(!fonts.isEnabled());
        });
        activate(hovered, 1, () -> {
            row.expanded = !row.expanded; editing = null;
            if (row.expanded && !search.isEmpty()) row.flash = System.currentTimeMillis() + 600;
        });
        float textY = fonts.centeredY(row.name, y, height) - bounce;
        text(row.name, x + options.modulePadding.get(), textY, options.color(isEnabled(row) ? options.enabledColor : options.textColor));
        if (options.gear.get()) {
            String gear = row.expanded ? "-" : "+";
            text(gear, x + width - 4 - fonts.width("+"), fonts.centeredY(gear, y, height) - bounce, options.color(options.textColor));
        }
        Keybind keybind = row.hud == null ? keybind(row) : null;
        if (options.binds.get() && keybind != null && keybind.getBoundKey().getValue() != -1) {
            String label = "[" + keybind.getDisplayName() + "]";
            fonts.draw(draw, label, x + options.modulePadding.get() + fonts.width(row.name), fonts.centeredY(label, y, height, .5f) - bounce, options.color(options.textColor), opacity, .5f);
        }
        if (row.opening.value() > .01f) {
            float savedBottom = clipBottom; clipBottom = Math.min(clipBottom, y + height + row.opening.value());
            draw.pushClipRect(x, Math.max(clipTop, y + height), x + width, Math.max(Math.max(clipTop, y + height), clipBottom), true);
            float offset = y + height;
            for (Setting<?> setting : row.settings) {
                if (setting.isVisible()) { renderControl(setting, x, offset, width, active && row.expanded); offset += controlHeight(setting); }
            }
            if (row.hud == null) renderBinding(row, x, offset, width, active && row.expanded);
            draw.popClipRect(); clipBottom = savedBottom;
        }
    }

    private Keybind keybind(Row row) {
        return row.id.equals("@ui") ? manager.getOpenKeybind() : row.id.equals("@fonts") ? manager.getFontKeybind()
                : row.id.equals("@hud") ? Myth.HUD_MANAGER.getKeybind()
                : Myth.MODEL_MANAGER.getKeybind(row.id).orElseThrow();
    }
    private void renderBinding(Row row, float x, float y, float width, boolean active) {
        Keybind keybind = keybind(row); boolean hovered = hit(x, y, width, rowHeight(), active);
        if (hovered) hoveredBinding = keybind;
        hover(row.id + "/key", hovered, "");
        activate(hovered, 0, () -> { binding = binding == keybind ? null : keybind; editing = null; });
        activate(hovered && binding == null, 1, keybind::cycleMode);
        String label = keybind.getDisplayLabel(binding == keybind);
        float bounce = row.keyBounce.update(hovered && options.bounce.get() ? 1 : 0, 4, delta, true);
        text(label, x + 4, fonts.centeredY(label, y, rowHeight()) - bounce, options.color(options.textColor));
    }

    @SuppressWarnings("unchecked")
    private void renderControl(Setting<?> value, float x, float y, float width, boolean active) {
        Control state = controls.computeIfAbsent(value, ignored -> new Control());
        float height = rowHeight(); boolean hovered = hit(x, y, width, height, active);
        hover("setting/" + System.identityHashCode(value), hovered, value.getDescription());
        float bounce = state.bounce.update(hovered && options.bounce.get() ? 1 : 0, 4, delta, true);
        String label = value.getName();
        switch (value.getKind()) {
            case BOOLEAN -> {
                var setting = (Setting<Boolean>) value;
                float fill = state.fill.update(setting.get() && !setting.isAction() ? 1 : 0, options.animSpeed.get() * 2, delta, true);
                if (fill > 0) rect(x + 1, y + .5f, x + Math.max(width * fill - 1, 2), y + height - .5f, options.themeColor());
                activate(hovered && !(setting.isAction() && setting.isGroup()), 0, () -> { setting.set(!setting.get()); editing = null; });
                activate(hovered && setting.isGroup(), 1, () -> setting.setExpanded(!setting.isExpanded()));
                if (setting.isGroup()) {
                    String gear = setting.isAction() ? "..." : setting.isExpanded() ? "-" : "+";
                    text(gear, x + width - 4 - fonts.width(setting.isAction() ? "..." : "+"), fonts.centeredY(gear, y, height) - bounce, options.color(options.textColor));
                }
            }
            case NUMBER -> {
                var setting = (Setting<Number>) value;
                activate(hovered, 0, () -> { slider = setting; editing = null; });
                if (slider == setting && ImGui.isMouseDown(0)) {
                    double number = setting.getMin() + Math.clamp((mx - x) / (width - 2), 0, 1) * (setting.getMax() - setting.getMin());
                    setting.setNumber(Math.round(number / setting.getStep()) * setting.getStep());
                }
                activate(hovered, 1, () -> beginEdit(setting));
                float target = (float) ((setting.get().doubleValue() - setting.getMin()) / (setting.getMax() - setting.getMin()));
                float fill = state.fill.update(target, 6.5f, delta, false);
                rect(x + 1, y + .5f, (int) (x + Math.clamp((width - 2) * fill, 0, width - 3) + 2), y + height - .5f, options.themeColor());
                label += ": " + (editing == setting ? editorText() : format(setting)) + setting.getUnit();
            }
            case STRING -> {
                var setting = (Setting<String>) value;
                rect(x + 1, y + .5f, x + width - 1, y + height - .5f, options.themeColor());
                activate(hovered, 0, () -> { if (editing == setting) editing = null; else beginEdit(setting); });
                label = editing == setting ? editorText() : setting.getName() + ": " + setting.get();
            }
            case ENUM -> {
                var setting = (Setting<Object>) value;
                int inset = state.open ? 1 : 0;
                rect(x + 1 + inset, y + .5f + inset, x + width - 1 - inset, y + height - .5f, options.themeColor());
                activate(hovered, 0, () -> setting.set(setting.getChoices().get((setting.getChoices().indexOf(setting.get()) + 1) % setting.getChoices().size())));
                activate(hovered, 1, () -> state.open = !state.open);
                label += ": " + setting.get();
                if (state.open) {
                    float offset = y + height;
                    for (Object choice : setting.getChoices()) {
                        boolean choiceHover = hit(x + 2, offset, width - 4, height, active);
                        hover("choice/" + System.identityHashCode(setting) + "/" + choice, choiceHover, "");
                        if (choice.equals(setting.get())) rect(x + 3, offset + .5f, x + width - 2, offset + height - .5f, options.themeColor());
                        activate(choiceHover, 0, () -> setting.set(choice));
                        text(choice.toString(), x + 4, fonts.centeredY(choice.toString(), offset, height), options.color(options.textColor)); offset += height;
                    }
                    if (options.line.get()) outline(x + 1, y + .5f, x + width - 2, offset - .5f, options.color(options.lineColor));
                }
            }
            case COLOR -> {
                var setting = (Setting<Integer>) value;
                activate(hovered, 1, () -> { state.open = !state.open; state.hsv = null; });
                rect(x + width - 12, y + 1, x + width - 3.5f, y + 10, options.themeColor());
                rect(x + width - 11.5f, y + 1.5f, x + width - 4, y + 9.5f, setting.color(options.themeColor()) | 0xff000000);
                if (state.open) renderColor(setting, x, y + height, width, active);
            }
            case SOUND -> {
                var setting = (Setting<String>) value;
                rect(x + 1, y + .5f, x + width - 1, y + height - .5f, options.themeColor());
                label += ": " + setting.get().substring(setting.get().lastIndexOf('/') + 1);
                activate(hovered, 0, () -> { soundPicker = "System"; soundSetting = setting; reloadSoundPanels(); });
            }
            case SET -> {
                var setting = (Setting<List<String>>) value;
                label += ": " + setting.get().size();
                activate(hovered, 0, () -> state.open = !state.open);
                activate(hovered, 1, () -> state.open = !state.open);
                if (state.open) {
                    float offset = y + height;
                    renderControl(state.filter, x + 1, offset, width - 2, active); offset += height;
                    List<String> matches = selectionMatches(setting, state);
                    int pages = Math.max(1, (matches.size() + 7) / 8); state.page = Math.clamp(state.page, 0, pages - 1);
                    for (String choice : matches.stream().skip(state.page * 8L).limit(8).toList()) {
                        boolean choiceHover = hit(x + 2, offset, width - 4, height, active);
                        if (setting.get().contains(choice)) rect(x + 2, offset + .5f, x + width - 2, offset + height - .5f, options.themeColor());
                        activate(choiceHover, 0, () -> {
                            var selected = new ArrayList<>(setting.get());
                            if (!selected.remove(choice)) selected.add(choice); setting.set(selected);
                        });
                        String name = choice.replace("minecraft:", "");
                        draw.pushClipRect(x + 2, Math.max(clipTop, offset), x + width - 2,
                                Math.max(Math.max(clipTop, offset), Math.min(clipBottom, offset + height)), true);
                        text(name, x + 4, fonts.centeredY(name, offset, height), options.color(options.textColor)); draw.popClipRect();
                        offset += height;
                    }
                    String page = "< " + (state.page + 1) + " / " + pages + " >";
                    text(page, x + 4, fonts.centeredY(page, offset, height), options.color(options.textColor));
                    activate(hit(x + 2, offset, width / 2 - 2, height, active), 0, () -> state.page = Math.floorMod(state.page - 1, pages));
                    activate(hit(x + width / 2, offset, width / 2 - 2, height, active), 0, () -> state.page = (state.page + 1) % pages);
                }
            }
        }
        draw.pushClipRect(x + 2, Math.max(clipTop, y), x + width - 2, Math.max(Math.max(clipTop, y), Math.min(clipBottom, y + height)), true);
        if (hovered) {
            if (state.hoveredSince == 0) state.hoveredSince = System.currentTimeMillis();
        } else state.hoveredSince = 0;
        float overflow = Math.max(0, fonts.width(label) - width + 6);
        float scroll = state.hoveredSince == 0 ? 0 : Math.clamp((System.currentTimeMillis() - state.hoveredSince - 750) / 500f, 0, 1) * overflow;
        text(label, x + 4 - scroll, fonts.centeredY(label, y, height) - bounce, options.color(options.textColor)); draw.popClipRect();
    }

    private void renderColor(Setting<Integer> setting, float x, float y, float width, boolean active) {
        Control state = controls.get(setting);
        int argb = setting.color(options.themeColor());
        if (state.hsv == null || state.lastColor != argb && colorDragging != setting) {
            state.hsv = Color.RGBtoHSB(argb >> 16 & 255, argb >> 8 & 255, argb & 255, null);
            state.saturation.snap(state.hsv[1] * (width - 8)); state.brightness.snap((1 - state.hsv[2]) * (width - 8));
            state.hue.snap(state.hsv[0] * (width - 2)); state.alpha.snap((width - 4) - ((width - 6) * (argb >>> 24) / 255f));
        }
        float[] hsv = state.hsv;
        state.lastColor = argb;
        float sx = x + 3, sy = y + 1, size = width - 4;
        float hueY = y + width, alphaY = hueY + 8;
        activate(hit(sx, sy, size, size, active), 0, () -> { colorDragging = setting; colorPart = 0; });
        activate(hit(sx, hueY, size, 6, active), 0, () -> { colorDragging = setting; colorPart = 1; });
        activate(setting.hasAlpha() && hit(sx, alphaY, size, 6, active), 0, () -> { colorDragging = setting; colorPart = 2; });
        if (colorDragging == setting && ImGui.isMouseDown(0)) {
            switch (colorPart) {
                case 0 -> { hsv[1] = Math.clamp((mx - sx - 2) / (size - 4), 0, 1); hsv[2] = 1 - Math.clamp((my - sy - 2) / (size - 4), 0, 1); }
                case 1 -> hsv[0] = Math.clamp((mx - sx) / size, 0, 1);
                case 2 -> argb = (argb & 0xffffff) | Math.round((1 - Math.clamp((mx - sx) / size, 0, 1)) * 255) << 24;
            }
            setting.set((argb & 0xff000000) | Color.HSBtoRGB(hsv[0], hsv[1], hsv[2]) & 0xffffff);
            argb = setting.get();
            state.lastColor = argb;
        }
        int border = colorDragging == setting ? 0xffffffff : options.themeColor() | 0xff000000;
        rect(sx - .5f, sy - .5f, sx + size + .5f, sy + size + .5f, border);
        draw.addRectFilledMultiColor(sx, sy, sx + size, sy + size, packed(0xffffffff), packed(Color.HSBtoRGB(hsv[0], 1, 1)), packed(0xff000000), packed(0xff000000));
        rect(sx - .5f, hueY - .5f, sx + size + .5f, hueY + 6.5f, border);
        for (int i = 0; i < 6; i++) draw.addRectFilledMultiColor(sx + size * i / 6, hueY, sx + size * (i + 1) / 6, hueY + 6,
                packed(Color.HSBtoRGB(i / 6f, 1, 1)), packed(Color.HSBtoRGB((i + 1) / 6f, 1, 1)), packed(Color.HSBtoRGB((i + 1) / 6f, 1, 1)), packed(Color.HSBtoRGB(i / 6f, 1, 1)));
        float hx = x + Math.clamp(state.hue.update(hsv[0] * (width - 2), 5, delta, true), 3, width - 3);
        rect((int) hx - .5f, hueY - .5f, (int) hx + 2.5f, hueY + 6.5f, border);
        rect((int) hx, hueY, (int) hx + 2, hueY + 6.5f, 0xffffffff);
        if (setting.hasAlpha()) {
            rect(sx - .5f, alphaY - .5f, sx + size + .5f, alphaY + 6.5f, border);
            for (int i = 0; i < size; i += 6) {
                rect(sx + i, alphaY, Math.min(sx + size, sx + i + 6), alphaY + 6, 0xffffffff);
                rect(sx + i, alphaY + 3, Math.min(sx + size, sx + i + 3), alphaY + 6, 0xff909090);
                rect(sx + i + 3, alphaY, Math.min(sx + size, sx + i + 6), alphaY + 3, 0xff909090);
            }
            int rgb = argb | 0xff000000;
            draw.addRectFilledMultiColor(sx, alphaY, sx + size, alphaY + 6, packed(rgb), packed(rgb & 0xffffff), packed(rgb & 0xffffff), packed(rgb));
            float ax = sx + state.alpha.update(size - ((size - 2) * (argb >>> 24) / 255f), 5, delta, true) - 1;
            rect(ax - 1.5f, alphaY - .5f, ax + 1.5f, alphaY + 6.5f, border);
            rect(ax - 1, alphaY, ax + 1, alphaY + 6, 0xffffffff);
        }
        float pointerX = x + 5 + state.saturation.update(hsv[1] * (width - 8), 5, delta, true);
        float pointerY = y + 3 + state.brightness.update((1 - hsv[2]) * (width - 8), 5, delta, true);
        float expansion = 2 * state.pointerSize.update(colorDragging == setting && colorPart == 0 ? 1 : 0, 4, delta, true);
        rect(pointerX - 1.5f - expansion, pointerY - 1.5f - expansion, pointerX + 1.5f + expansion, pointerY + 1.5f + expansion, border);
        int pointerColor = Color.HSBtoRGB(hsv[0], hsv[1], hsv[2]);
        if (colorDragging != setting) pointerColor = new Color(pointerColor).darker().getRGB();
        rect(pointerX - 1 - expansion, pointerY - 1 - expansion, pointerX + 1 + expansion, pointerY + 1 + expansion, pointerColor);
        float offset = y + width + 6 + (setting.hasAlpha() ? 10 : 2);
        boolean copy = hit(x + 2, offset, width / 2 - 2, rowHeight(), active);
        boolean paste = hit(x + width / 2, offset, width / 2 - 2, rowHeight(), active);
        rect(x + 2, offset, x + width / 2 - .5f, offset + rowHeight() - .5f, options.themeColor());
        rect(x + width / 2 + .5f, offset, x + width - .5f, offset + rowHeight() - .5f, options.themeColor());
        long now = System.currentTimeMillis();
        String copyLabel = now - state.copied < 1000 ? "Copied" : "Copy";
        String pasteLabel = now - state.invalid < 1000 ? "Invalid" : now - state.pasted < 1000 ? "Pasted" : "Paste";
        text(copyLabel, x + width / 4 - fonts.width(copyLabel) / 2, fonts.centeredY(copyLabel, offset, rowHeight()), options.color(options.textColor));
        text(pasteLabel, x + width * .75f - fonts.width(pasteLabel) / 2, fonts.centeredY(pasteLabel, offset, rowHeight()), options.color(options.textColor));
        int clipboardColor = argb;
        activate(copy, 0, () -> { Minecraft.getInstance().keyboardHandler.setClipboard(String.format(Locale.ROOT, "#%08X", clipboardColor)); state.copied = now; });
        activate(paste, 0, () -> {
            try {
                String value = Minecraft.getInstance().keyboardHandler.getClipboard().trim().replace("#", "");
                if (!value.matches("[0-9a-fA-F]{6}|[0-9a-fA-F]{8}")) throw new IllegalArgumentException("Invalid color");
                long parsed = Long.parseLong(value, 16);
                setting.set((int) parsed | (value.length() <= 6 ? 0xff000000 : 0));
                state.hsv = null; state.pasted = now;
            } catch (RuntimeException ignored) { state.invalid = now; Myth.LOGGER.debug("Invalid color in clipboard"); }
        });
        offset += rowHeight() + 1;
        if (setting.canSync()) {
            colorToggle(setting.getId() + "/sync", "Sync", setting.isSync(), x, offset, width, active, () -> setting.setSync(!setting.isSync()));
            offset += rowHeight() + .5f;
        }
        colorToggle(setting.getId() + "/rainbow", "Rainbow", setting.isRainbow(), x, offset, width, active, () -> setting.setRainbow(!setting.isRainbow()));
    }

    private void colorToggle(String id, String label, boolean enabled, float x, float y, float width, boolean active, Runnable change) {
        if (enabled) rect(x + 2, y + .5f, x + width - 1, y + rowHeight() - .5f, options.themeColor());
        text(label, x + width / 2 - fonts.width(label) / 2, fonts.centeredY(label, y, rowHeight()), options.color(options.textColor));
        hover("color/" + id, hit(x, y, width, rowHeight(), active), "");
        activate(hit(x, y, width, rowHeight(), active), 0, change);
    }

    private void renderSoundPicker() {
        float width = options.width();
        if (draggingSound != null) {
            if (ImGui.isMouseDown(0)) {
                draggingSound.x = Math.clamp(mx - dragX, 0, Math.max(0, screenWidth - width));
                draggingSound.y = Math.clamp(my - dragY, 0, Math.max(0, screenHeight - (draggingSound.expanded ? 15 + rowHeight() * draggingSound.entries.size() : 14)));
            } else draggingSound = null;
        }
        for (SoundPanel panel : soundPanels) manager.drawPanelBlur(draw, panel.x, panel.y, width,
                panel.expanded ? 15.5f + panel.entries.size() * rowHeight() : 14);
        for (SoundPanel panel : soundPanels) {
            float x = panel.x, y = panel.y;
            rect(x, y, x + width, y + 14, options.color(options.bgColor));
            rect(x, y, x + width, y + 14, options.themeColor());
            String title = panel.name.substring(0, 1).toUpperCase(Locale.ROOT) + panel.name.substring(1);
            manager.getIcons().drawTitle(draw, "sounds", title, fonts, x, y, width, opacity);
            if (inside(x, y, width, 14)) {
                click(true, 0, () -> { draggingSound = panel; dragX = mx - x; dragY = my - y; });
                click(true, 1, () -> panel.expanded = !panel.expanded);
            }
            float offset = y + 15;
            if (panel.expanded) {
                rect(x, y + 14, x + width, offset + panel.entries.size() * rowHeight() + .5f, options.color(options.bgColor));
                for (UiSounds.Entry entry : panel.entries) {
                    rect(x + 1, offset + .5f, x + width - 1, offset + rowHeight() - .5f, entry.key().equals(soundSetting.get()) ? options.themeColor() : options.color(options.bgButton));
                    boolean hovered = inside(x, offset, width, rowHeight());
                    text(entry.name(), x + 4, fonts.centeredY(entry.name(), offset, rowHeight()) - (hovered && options.bounce.get() ? 1 : 0), options.color(options.textColor));
                    hover("sound/" + entry.key(), hovered, "");
                    if (hovered && !manager.isClosing()) {
                        click(true, 0, () -> { soundSetting.set(entry.key()); soundPicker = null; draggingSound = null; });
                        click(true, 1, () -> UiSounds.play(entry.key(), 1));
                    }
                    offset += rowHeight();
                }
            }
            if (options.line.get()) panelOutline(x - 1, y - 1, x + width, panel.expanded ? offset + .5f : y + 14);
        }
    }
    private void reloadSoundPanels() {
        soundPanels.clear();
        for (UiSounds.Entry entry : UiSounds.reload()) {
            SoundPanel panel = soundPanels.stream().filter(p -> p.name.equals(entry.group())).findFirst().orElse(null);
            if (panel == null) { panel = new SoundPanel(entry.group(), 5 + soundPanels.size() * (options.width() + 3)); soundPanels.add(panel); }
            panel.entries.add(entry);
        }
    }
    public void filesDropped(List<Path> paths) {
        if (soundPicker == null) return;
        UiSounds.importFiles(paths); reloadSoundPanels();
    }

    private void renderSearch() {
        if (!searchFocused) {
            String help = "Ctrl + F to activate search.";
            text(help, screenWidth - fonts.width(help), fonts.centeredY(help, screenHeight - fonts.height(), fonts.height()), 0xffb0b0b0); return;
        }
        suggestion = rows.values().stream().map(row -> row.name).filter(name -> !search.isEmpty()
                && name.toLowerCase(Locale.ROOT).startsWith(search.toLowerCase(Locale.ROOT))).findFirst().orElse("");
        String tail = suggestion.isEmpty() ? "" : suggestion.substring(search.length());
        String cursor = System.currentTimeMillis() % 1000 < 500 ? "_" : "";
        float x = screenWidth / 2 - fonts.width(search + tail + "_") / 2;
        float textY = fonts.centeredY(search.isEmpty() ? "Ag" : search + tail, screenHeight / 2 - fonts.height() / 2, fonts.height());
        text(search, x, textY, 0xffb0b0b0);
        text(tail, x + fonts.width(search), textY, 0xffffffff);
        text(cursor, x + fonts.width(search), textY, 0xffffffff);
    }
    private void renderTooltip() {
        if (tooltip.isBlank()) return;
        if (!hoveredNow.equals(tooltipOwner)) { tooltipOwner = hoveredNow; if (options.constantReset.get() || tooltipStarted == 0) tooltipStarted = System.currentTimeMillis(); }
        if (!options.descriptions.get() || System.currentTimeMillis() - tooltipStarted < options.delay.get()) return;
        String[] lines = tooltip.split("\n"); float width = 0;
        for (String line : lines) width = Math.max(width, fonts.width(line));
        float x = mx + 9; if (x + width + 1 > screenWidth) x = mx - width - 2;
        float y = Math.min(my, screenHeight - lines.length * (fonts.height() + 1));
        manager.drawBlur(draw, x, y - 1, width + 1, lines.length * (fonts.height() + 1) + 1);
        rect(x, y - 1, x + width + 1, y + lines.length * (fonts.height() + 1), 0x500a0a0a);
        for (int i = 0; i < lines.length; i++) text(lines[i], x + 1, fonts.centeredY(lines[i], y + i * (fonts.height() + 1), fonts.height() + 1), 0xffffffff);
    }
    private void renderSnow() {
        if (!UiOptions.isWinter() || !options.snow.get()) return;
        for (Snow flake : snow) {
            flake.progress = Math.min(1, flake.progress + delta * 2 * flake.speed);
            if (flake.progress >= 0) fonts.draw(draw, "\u2746", (flake.x + flake.progress * flake.drift) * screenWidth,
                    flake.progress * screenHeight * .5f - 10, Math.max(4, (int) (255 * (1 - flake.progress))) << 24 | 0xffffff, opacity, flake.scale);
        }
        snow.removeIf(flake -> flake.progress >= 1); while (snow.size() < 25) snow.add(new Snow());
    }

    private boolean inside(float x, float y, float width, float height) { return mx > x && mx < x + width && my > y && my < y + height; }
    private boolean hit(float x, float y, float width, float height, boolean active) { return active && my >= clipTop && my <= clipBottom && inside(x, y, width, height); }
    private void hover(String id, boolean hovered, String description) {
        if (!hovered) return;
        hoveredNow = id; tooltip = description;
        currentHover.add(id);
    }
    private void click(boolean hovered, int button, Runnable action) {
        if (!hovered || manager.isClosing() || clickUsed[button] || !manager.isMouseClicked(button)) return;
        clickUsed[button] = true;
        inputActions.add(action);
    }
    private void activate(boolean hovered, int button, Runnable action) {
        click(hovered, button, () -> { sound(button); action.run(); });
    }
    private void finishInput() {
        // Mio stores a hover flag for each control, rather than comparing every
        // control with one shared last-hovered ID during the drawing pass.
        if (!manager.isClosing() && options.hover.get() && currentHover.stream().anyMatch(id -> !previousHover.contains(id)))
            UiSounds.play(options.hoverSound.get(), options.hoverVolume.get());
        previousHover.clear(); previousHover.addAll(currentHover);
        // Complete the frame before changing visibility, fonts or positions.
        // One physical press is consumed by one control, even if layout changes.
        List<Runnable> actions = List.copyOf(inputActions); inputActions.clear();
        for (Runnable action : actions) action.run();
    }
    private void sound(int button) {
        if (button == 0 && options.leftClick.get()) UiSounds.play(options.clickSound.get(), options.clickVolume.get());
        else if (button == 1 && options.rightClick.get()) UiSounds.play(options.rightSound.get(), options.rightVolume.get());
    }
    private int packed(int argb) { return Math.clamp(Math.round((argb >>> 24) * opacity), 0, 255) << 24 | (argb & 255) << 16 | argb & 0xff00 | argb >> 16 & 255; }
    private void rect(float x, float y, float endX, float endY, int argb) { MioDraw.rect(draw, x, y, endX, endY, packed(argb)); }
    private void outline(float x, float y, float endX, float endY, int argb) { MioDraw.outline(draw, x, y, endX, endY, packed(argb)); }
    private void panelOutline(float x, float y, float endX, float endY) {
        MioDraw.panelOutline(draw, x, y, endX, endY, packed(options.color(options.bgColor)), packed(options.color(options.lineColor)));
    }
    private void text(String text, float x, float y, int argb) { fonts.draw(draw, text, x, y, argb, opacity); }
    private static String format(Setting<? extends Number> setting) {
        Number number = setting.get();
        if (number instanceof Integer) return number.toString();
        int decimals = Math.max(1, java.math.BigDecimal.valueOf(setting.getStep()).stripTrailingZeros().scale());
        return String.format(Locale.ROOT, "%." + decimals + "f", number.doubleValue());
    }
    private void beginEdit(Setting<?> setting) { editing = setting; binding = null; editBuffer = setting.get().toString(); editCursor = editBuffer.length(); resetSearch(); }
    private String editorText() { return editBuffer.substring(0, editCursor) + (System.currentTimeMillis() % 1000 < 500 ? "_" : "") + editBuffer.substring(editCursor); }
    private void resetSearch() { searchFocused = false; search = ""; suggestion = ""; }

    public boolean key(KeyEvent event) {
        if (binding != null) {
            binding.setKey(event.isEscape() || event.key() == GLFW.GLFW_KEY_DELETE ? InputConstants.UNKNOWN : InputConstants.getKey(event));
            binding = null; manager.save(); return true;
        }
        if (soundPicker != null) { if (event.isEscape()) soundPicker = null; return true; }
        if (editing != null) { editKey(event); return true; }
        if (manager.isHudEditorOpen()) return false;
        if (event.key() == GLFW.GLFW_KEY_F && event.hasControlDown()) { searchFocused = !searchFocused; if (!searchFocused) resetSearch(); return true; }
        if (searchFocused) {
            switch (event.key()) {
                case GLFW.GLFW_KEY_ESCAPE -> resetSearch();
                case GLFW.GLFW_KEY_BACKSPACE, GLFW.GLFW_KEY_DELETE -> { if (!search.isEmpty()) search = search.substring(0, search.length() - 1); }
                case GLFW.GLFW_KEY_TAB -> { if (!suggestion.isEmpty()) search = suggestion; }
                case GLFW.GLFW_KEY_ENTER -> {
                    String target = suggestion.isEmpty() ? search : suggestion;
                    for (Row row : rows.values()) if (row.name.equalsIgnoreCase(target)) { row.expanded = true; row.flash = System.currentTimeMillis() + 600; }
                    resetSearch();
                }
                default -> {}
            }
            return true;
        }
        return false;
    }
    public void keyRepeat(KeyEvent event) {
        if (editing != null) editKey(event);
        else if (searchFocused && (event.key() == GLFW.GLFW_KEY_BACKSPACE || event.key() == GLFW.GLFW_KEY_DELETE)) key(event);
    }
    @SuppressWarnings("unchecked")
    private void editKey(KeyEvent event) {
        switch (event.key()) {
            case GLFW.GLFW_KEY_ESCAPE -> editing = null;
            case GLFW.GLFW_KEY_ENTER, GLFW.GLFW_KEY_KP_ENTER -> {
                try {
                    if (editing.getKind() == Setting.Kind.NUMBER) editing.setNumber(Double.parseDouble(editBuffer));
                    else ((Setting<String>) editing).set(editBuffer);
                    editing = null;
                } catch (IllegalArgumentException ignored) { Myth.LOGGER.debug("Invalid value in UI editor"); }
            }
            case GLFW.GLFW_KEY_BACKSPACE -> { if (editCursor > 0) { editBuffer = editBuffer.substring(0, editCursor - 1) + editBuffer.substring(editCursor); editCursor--; } }
            case GLFW.GLFW_KEY_DELETE -> { if (editCursor < editBuffer.length()) editBuffer = editBuffer.substring(0, editCursor) + editBuffer.substring(editCursor + 1); }
            case GLFW.GLFW_KEY_LEFT -> editCursor = Math.max(0, editCursor - 1);
            case GLFW.GLFW_KEY_RIGHT -> editCursor = Math.min(editBuffer.length(), editCursor + 1);
            case GLFW.GLFW_KEY_HOME -> editCursor = 0;
            case GLFW.GLFW_KEY_END -> editCursor = editBuffer.length();
            case GLFW.GLFW_KEY_V -> { if (event.hasControlDown()) insert(Minecraft.getInstance().keyboardHandler.getClipboard()); }
            case GLFW.GLFW_KEY_C -> { if (event.hasControlDown()) Minecraft.getInstance().keyboardHandler.setClipboard(editBuffer); }
            default -> {}
        }
    }
    public void character(int codepoint) {
        if (manager.isClosing()) return;
        String value = Character.toString(codepoint);
        if (editing != null) insert(value);
        else if (searchFocused && search.length() < 64 && !Character.isISOControl(codepoint)) search += value;
    }
    private void insert(String text) {
        if (text.length() + editBuffer.length() > 4096) return;
        if (editing.getKind() == Setting.Kind.NUMBER && !text.matches("[0-9.\\-+eE]+")) return;
        editBuffer = editBuffer.substring(0, editCursor) + text + editBuffer.substring(editCursor); editCursor += text.length();
    }
    public boolean captureMouse(int button) {
        // Mio reserves left click for entering/leaving capture; other buttons bind.
        if (binding == null || binding != hoveredBinding || button == GLFW.GLFW_MOUSE_BUTTON_LEFT) return false;
        binding.setKey(InputConstants.Type.MOUSE.getOrCreate(button)); binding = null; manager.save(); return true;
    }
    public void scroll(double y) {
        int amount = (int) (y * rowHeight());
        if (soundPicker != null) { for (SoundPanel panel : soundPanels) if (panel != draggingSound) panel.y += amount; return; }
        if (amount != 0) options.position.set("Custom");
        for (Panel panel : panels) if (panel != dragging) panel.y += amount;
    }
}
