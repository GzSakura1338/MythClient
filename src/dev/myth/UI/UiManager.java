package dev.myth.UI;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.mojang.blaze3d.platform.InputConstants;
import dev.myth.Myth;
import dev.myth.UI.font.FontManager;
import dev.myth.UI.hud.HudEditor;
import dev.myth.UI.hud.HudEditorScreen;
import dev.myth.UI.setting.Setting;
import dev.myth.event.api.EventType;
import dev.myth.event.api.annotations.EventTarget;
import dev.myth.event.impl.GameTickEvent;
import dev.myth.model.Model;
import dev.myth.utils.ChatUtils;
import dev.myth.utils.Keybind;
import java.io.IOException;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import net.minecraft.client.KeyMapping;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.screens.ChatScreen;
import net.minecraft.client.gui.screens.Screen;
import net.minecraft.client.gui.screens.TitleScreen;
import net.minecraft.client.input.KeyEvent;
import org.lwjgl.glfw.GLFW;

public final class UiManager {
    public record SettingState(Setting<?> setting, Setting.State state) {}
    public record PanelState(CategoryName category, float x, float y, boolean expanded, int order) {}
    public enum CategoryName { COMBAT, MISC, RENDER, PLAYER, CLIENT }
    public record State(List<SettingState> settings, List<PanelState> panels, Map<String, Boolean> expandedRows,
                        InputConstants.Key openKey, Keybind.Mode openMode, InputConstants.Key fontKey, Keybind.Mode fontMode, boolean fontEnabled) {}

    private final UiOptions options = new UiOptions();
    private final FontManager fonts = new FontManager();
    private final CategoryIcons icons = new CategoryIcons();
    private final ClickGui gui = new ClickGui(this, options, fonts);
    private final ImGuiRenderer renderer = new ImGuiRenderer();
    private final HudEditor hudEditor = new HudEditor(Myth.HUD_MANAGER, icons);
    private Keybind openKey;
    private Keybind fontKey;
    private boolean initialized;
    private boolean closing;
    private long transition;
    private String failure;
    private JsonObject persisted = new JsonObject();

    public void initialize() {
        if (initialized) return;
        // Openmio UI.getKeybind always coerces the opening binding to Toggle.
        openKey = new Keybind("clickgui", GLFW.GLFW_KEY_RIGHT_SHIFT, true);
        fontKey = new Keybind("gui_fonts", Keybind.UNBOUND);
        Myth.HUD_MANAGER.hudEditor.onChange(value -> { if (value) openHudEditor(); });
        options.reset.onChange(value -> { if (value) gui.resetLayout(); });
        options.frameWidth.onChange(value -> gui.resetLayout());
        options.position.onChange(value -> { if (!value.equals("Custom")) gui.resetLayout(); });
        options.prefix.onChange(value -> {
            if (value.isBlank() || value.length() > 8 || value.chars().anyMatch(Character::isWhitespace))
                throw new IllegalArgumentException("Prefix must contain 1 to 8 non-space characters");
        });
        Myth.EVENT_MANAGER.register(this); initialized = true;
    }

    public Keybind getOpenKeybind() { return openKey; }
    public CategoryIcons getIcons() { return icons; }
    public void drawBlur(imgui.ImDrawList draw, float x, float y, float width, float height) {
        if (options.blur.get()) renderer.drawBlur(draw, x, y, width, height, opacity());
    }
    public Keybind getFontKeybind() { return fontKey; }
    public String getCommandPrefix() { return options.prefix.get(); }
    public boolean isClosing() { return closing; }
    public boolean isOpen() { return Minecraft.getInstance().gui.screen() instanceof ClickGuiScreen; }
    public boolean isHudEditorOpen() { return Minecraft.getInstance().gui.screen() instanceof HudEditorScreen; }
    private boolean isEditing() { return isOpen() || isHudEditorOpen(); }
    public float opacity() {
        float progress = Math.clamp((System.currentTimeMillis() - transition) / 150f, 0, 1);
        return closing ? 1 - progress : progress;
    }

    public void open() {
        Minecraft client = Minecraft.getInstance();
        if (isOpen() || client.gui.overlay() != null) return;
        if (!client.getWindow().isMinimized() && !client.getWindow().isIconified()) {
            if (!com.mojang.blaze3d.systems.RenderSystem.getDevice().getDeviceInfo().backendName().equals("OpenGL")) {
                failure = "ClickGUI requires the OpenGL graphics backend.";
                Myth.LOGGER.error(failure); ChatUtils.addChatMessage("§c" + failure); return;
            }
        }
        try {
            if (!renderer.initialize(client.getWindow().handle())) return;
            fonts.prepare(); renderer.clearInput(); renderer.invalidateBlur(); gui.opened();
            closing = false; transition = System.currentTimeMillis(); failure = null;
            Screen previous = client.gui.screen();
            KeyMapping.releaseAll();
            client.gui.setScreen(new ClickGuiScreen(this, previous));
        } catch (RuntimeException | LinkageError exception) {
            failure = "Could not initialize ClickGUI. See the game log.";
            Myth.LOGGER.error(failure, exception); ChatUtils.addChatMessage("§c" + failure);
        }
    }

    public boolean key(long window, int action, KeyEvent key) {
        Minecraft client = Minecraft.getInstance();
        if (!initialized || window != client.getWindow().handle()) return false;
        if (isEditing()) {
            renderer.key(key.key(), key.scancode(), action, key.modifiers());
            if (action == GLFW.GLFW_PRESS && !closing) {
                boolean handled = isHudEditorOpen() ? hudEditor.key(key) : gui.key(key);
                if (!handled && (key.isEscape() || openKey.getMapping().matches(key))) requestClose();
            } else if (action == GLFW.GLFW_REPEAT && !closing) {
                if (isHudEditorOpen()) hudEditor.key(key); else gui.keyRepeat(key);
            }
            return true;
        }
        Screen screen = client.gui.screen();
        if (action == GLFW.GLFW_PRESS
                && openKey.getMapping().matches(key) && client.isWindowActive()
                && client.gui.overlay() == null && (screen == null || screen instanceof TitleScreen || screen instanceof ChatScreen)) {
            open(); return isOpen();
        }
        return false;
    }
    public boolean button(long window, int button, int action, int mods) {
        Minecraft client = Minecraft.getInstance();
        if (!initialized || window != client.getWindow().handle()) return false;
        if (!isEditing()) {
            if (action == GLFW.GLFW_PRESS
                    && openKey.getBoundKey().getType() == InputConstants.Type.MOUSE
                    && openKey.getBoundKey().getValue() == button && client.isWindowActive() && client.gui.overlay() == null
                    && (client.gui.screen() == null || client.gui.screen() instanceof TitleScreen)) { open(); return isOpen(); }
            return false;
        }
        if (closing) {
            renderer.button(button, action, mods);
            return true;
        }
        if (isOpen() && action == GLFW.GLFW_PRESS && gui.captureMouse(button)) return true;
        if (openKey.getBoundKey().getType() == InputConstants.Type.MOUSE && openKey.getBoundKey().getValue() == button
                && action == GLFW.GLFW_PRESS) { requestClose(); return true; }
        renderer.button(button, action, mods); return true;
    }
    public void character(int codepoint) { if (isOpen()) { gui.character(codepoint); renderer.character(codepoint); } }
    public void scroll(double x, double y) {
        if (closing) return;
        renderer.scroll(x, y); if (isHudEditorOpen()) hudEditor.scroll(y); else gui.scroll(y);
    }
    public void filesDropped(List<Path> paths) { if (isOpen()) gui.filesDropped(paths); }
    public void requestClose() {
        if (!closing) { closing = true; transition = System.currentTimeMillis(); hudEditor.resetInput(); }
    }
    public void tickScreen(Screen screen, Screen previous) {
        if (closing && System.currentTimeMillis() - transition >= 150 && Minecraft.getInstance().gui.screen() == screen) {
            Minecraft.getInstance().gui.setScreen(previous);
            closing = false; transition = System.currentTimeMillis();
            if (previous instanceof ClickGuiScreen) gui.opened();
        }
    }
    public void screenRemoved() { renderer.clearInput(); gui.removed(); hudEditor.resetInput(); KeyMapping.releaseAll(); save(); }

    public void openHudEditor() {
        Minecraft client = Minecraft.getInstance();
        if (!initialized || isHudEditorOpen() || client.gui.overlay() != null) return;
        try {
            if (!com.mojang.blaze3d.systems.RenderSystem.getDevice().getDeviceInfo().backendName().equals("OpenGL"))
                throw new IllegalStateException("HUD editor requires the OpenGL graphics backend");
            if (!renderer.initialize(client.getWindow().handle())) return;
            fonts.prepare(); renderer.clearInput(); renderer.invalidateBlur(); hudEditor.resetInput();
            KeyMapping.releaseAll();
            client.gui.setScreen(new HudEditorScreen(this, client.gui.screen()));
            closing = false; transition = System.currentTimeMillis(); failure = null;
        } catch (RuntimeException | LinkageError exception) {
            Myth.LOGGER.error("Could not open HUD editor", exception);
            ChatUtils.addChatMessage("§cCould not open HUD editor. See the game log.");
        }
    }

    /** Called after Minecraft has blitted its main target and just before the surface swap. */
    public void render() {
        Minecraft client = Minecraft.getInstance();
        if (initialized && fontKey.canUpdateHeldState(fonts.isEnabled(), client.gui.screen() != null))
            fonts.setEnabled(fontKey.getMode().heldState(fontKey.isPressed()));
        Myth.HUD_MANAGER.updateHeldKeybind();
        if ((!isEditing() && !Myth.HUD_MANAGER.shouldRender(client)) || client.gui.overlay() != null || failure != null) return;
        try {
            if (!renderer.initialize(client.getWindow().handle())) return;
            // Mio forces base GUI scale 2 independently of Minecraft's selected GUI scale.
            float physicalScale = client.getWindow().calculateScale(2, client.isEnforceUnicode()) * options.guiScale.get();
            renderer.configureBlur(isEditing() && options.blur.get(), options.blurStrength.get(), options.blurFps.get());
            renderer.render(() -> {
                fonts.prepare();
                if (isOpen()) gui.render();
                else if (isHudEditorOpen()) hudEditor.render(options, fonts, opacity(), !closing);
                else {
                    float width = imgui.ImGui.getIO().getDisplaySizeX(), height = imgui.ImGui.getIO().getDisplaySizeY();
                    Myth.HUD_MANAGER.layout(fonts, width, height);
                    Myth.HUD_MANAGER.render(fonts, width, height, 1, false);
                }
            }, physicalScale, options.samples());
        } catch (RuntimeException | LinkageError exception) {
            failure = "Could not render ClickGUI. See the game log.";
            Myth.LOGGER.error(failure, exception); requestClose();
        }
    }

    @EventTarget private void onTick(GameTickEvent event) {
        if (event.getType() != EventType.POST || !initialized) return;
        openKey.discardPresses();
        Minecraft client = Minecraft.getInstance();
        if (!client.isWindowActive()) { openKey.release(); fontKey.release(); return; }
        if (fontKey.getMode() == Keybind.Mode.TOGGLE) {
            if (client.gui.screen() == null) while (fontKey.consumePress()) fonts.setEnabled(!fonts.isEnabled());
            else fontKey.discardPresses();
        } else {
            fontKey.discardPresses();
        }
    }

    public void toggle(Model model) {
        try { model.toggle(); } catch (RuntimeException exception) { Myth.LOGGER.error("Could not toggle {}", model.getId(), exception); }
    }
    public void save() {
        try { Myth.CONFIG_MANAGER.saveConfig(); }
        catch (IOException | RuntimeException exception) { Myth.LOGGER.error("Could not save UI config", exception); }
    }

    public JsonObject capture() {
        JsonObject root = persisted.deepCopy();
        root.addProperty("key", openKey.serialize()); root.addProperty("fontKey", fontKey.serialize());
        root.addProperty("keyMode", openKey.getMode().getId()); root.addProperty("fontMode", fontKey.getMode().getId());
        root.addProperty("fontEnabled", fonts.isEnabled());
        root.add("options", captureSettings(root.getAsJsonObject("options"), options.getSettings()));
        root.add("fonts", captureSettings(root.getAsJsonObject("fonts"), fonts.getSettings()));
        JsonObject panels = new JsonObject();
        int order = 0;
        for (ClickGui.Panel panel : gui.getPanels()) {
            JsonObject state = new JsonObject(); state.addProperty("x", panel.x); state.addProperty("y", panel.y); state.addProperty("expanded", panel.expanded);
            state.addProperty("order", order++);
            panels.add(panel.category.getId(), state);
        }
        root.add("panels", panels);
        root.addProperty("panelGap", ClickGui.PANEL_GAP);
        JsonObject rows = new JsonObject(); gui.getExpandedRows().forEach(rows::addProperty); root.add("rows", rows);
        return root;
    }
    private JsonObject captureSettings(JsonObject previous, List<Setting<?>> settings) {
        JsonObject data = previous == null ? new JsonObject() : previous.deepCopy();
        for (Setting<?> setting : settings) data.add(setting.getId(), setting.toJson());
        return data;
    }

    public State decode(JsonObject root) {
        List<SettingState> settings = new ArrayList<>();
        decodeSettings(root.getAsJsonObject("options"), options.getSettings(), settings);
        decodeSettings(root.getAsJsonObject("fonts"), fonts.getSettings(), settings);
        for (SettingState state : settings) if (state.setting() == options.prefix) {
            String value = (String) state.state().value();
            if (value.isBlank() || value.length() > 8 || value.chars().anyMatch(Character::isWhitespace)) throw new IllegalArgumentException("Invalid command prefix");
        }
        List<PanelState> panels = new ArrayList<>();
        if (root.has("panels")) for (ClickGui.Panel panel : gui.getPanels()) {
            var element = root.getAsJsonObject("panels").get(panel.category.getId());
            if (element == null) continue;
            JsonObject data = element.getAsJsonObject();
            float x = data.has("x") ? data.get("x").getAsFloat() : panel.x;
            float y = data.has("y") ? data.get("y").getAsFloat() : panel.y;
            if (!Float.isFinite(x) || !Float.isFinite(y) || Math.abs(x) > 100000 || Math.abs(y) > 100000) throw new IllegalArgumentException("Invalid panel position");
            int order = data.has("order") ? data.get("order").getAsInt() : gui.getPanels().indexOf(panel);
            if (order < 0 || order >= gui.getPanels().size()) throw new IllegalArgumentException("Invalid panel order");
            panels.add(new PanelState(CategoryName.valueOf(panel.category.name()), x, y, bool(data, "expanded", panel.expanded), order));
        }
        // Upgrade the previous automatic grid once; retain individually dragged layouts.
        if (!root.has("panelGap") && panels.size() == gui.getPanels().size()) {
            float savedWidth = options.width();
            for (SettingState value : settings) if (value.setting() == options.frameWidth)
                savedWidth = 92 + ((Number) value.state().value()).floatValue();
            List<PanelState> horizontal = panels.stream().sorted(java.util.Comparator.comparingDouble(PanelState::x)).toList();
            boolean legacyGrid = true;
            for (int i = 0; i < horizontal.size(); i++)
                legacyGrid &= Math.abs(horizontal.get(i).x() - (10 + i * (savedWidth + 3))) < .01f;
            if (legacyGrid) {
                panels = new ArrayList<>();
                for (int i = 0; i < horizontal.size(); i++) {
                    PanelState saved = horizontal.get(i);
                    panels.add(new PanelState(saved.category(), 10 + i * (savedWidth + ClickGui.PANEL_GAP),
                            saved.y(), saved.expanded(), saved.order()));
                }
            }
        }
        Map<String, Boolean> rows = new LinkedHashMap<>();
        if (root.has("rows")) root.getAsJsonObject("rows").entrySet().forEach(entry -> rows.put(entry.getKey(), bool(root.getAsJsonObject("rows"), entry.getKey(), false)));
        return new State(List.copyOf(settings), List.copyOf(panels), Map.copyOf(rows),
                root.has("key") ? Keybind.parseSavedKey(root.get("key").getAsString()) : openKey.getBoundKey(),
                root.has("keyMode") ? Keybind.Mode.parse(root.get("keyMode").getAsString()) : openKey.getMode(),
                root.has("fontKey") ? Keybind.parseSavedKey(root.get("fontKey").getAsString()) : fontKey.getBoundKey(),
                root.has("fontMode") ? Keybind.Mode.parse(root.get("fontMode").getAsString()) : fontKey.getMode(), bool(root, "fontEnabled", fonts.isEnabled()));
    }
    private void decodeSettings(JsonObject data, List<Setting<?>> settings, List<SettingState> states) {
        if (data == null) return;
        for (Setting<?> setting : settings) if (data.has(setting.getId())) states.add(new SettingState(setting, setting.decode(data.get(setting.getId()))));
    }
    private static boolean bool(JsonObject root, String key, boolean fallback) {
        JsonElement value = root.get(key); if (value == null) return fallback;
        if (!value.isJsonPrimitive() || !value.getAsJsonPrimitive().isBoolean()) throw new IllegalArgumentException("Expected boolean: " + key);
        return value.getAsBoolean();
    }
    public void apply(State state, JsonObject source) {
        for (SettingState value : state.settings()) value.setting().apply(value.state());
        for (PanelState saved : state.panels()) for (ClickGui.Panel panel : gui.getPanels()) if (panel.category.name().equals(saved.category().name())) {
            panel.x = saved.x(); panel.y = saved.y(); panel.expanded = saved.expanded();
        }
        gui.applyPanelOrder(state.panels().stream().sorted(java.util.Comparator.comparingInt(PanelState::order)).map(PanelState::category).toList());
        gui.applyExpandedRows(state.expandedRows()); fonts.setEnabled(state.fontEnabled());
        openKey.setKey(state.openKey()); openKey.setMode(state.openMode());
        fontKey.setKey(state.fontKey()); fontKey.setMode(state.fontMode()); persisted = source.deepCopy();
    }
    public void stop() { icons.close(); fonts.close(); renderer.close(); }
}
