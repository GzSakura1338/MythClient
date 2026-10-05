package dev.myth.UI.hud;

import com.google.gson.JsonObject;
import com.mojang.blaze3d.platform.InputConstants;
import dev.myth.Myth;
import dev.myth.UI.font.FontManager;
import dev.myth.UI.setting.Setting;
import dev.myth.event.api.EventType;
import dev.myth.event.api.annotations.EventTarget;
import dev.myth.event.impl.GameTickEvent;
import dev.myth.utils.Keybind;
import imgui.ImGui;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.EnumMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import net.minecraft.client.Minecraft;

/** Mio HUD registry, docking order and normalized placement, initially empty. */
public final class HudManager {
    public record SettingState(Setting<?> setting, Setting.State state) {}
    public record ElementState(HudElement element, boolean enabled, HudAnchor anchor, int index,
                               float x, float y, List<SettingState> settings) {}
    public record State(boolean enabled, InputConstants.Key key, Keybind.Mode mode, List<SettingState> settings,
                        float panelX, float panelY, boolean panelExpanded, List<ElementState> elements) {}
    public final Setting<Boolean> hudEditor = Setting.bool("hudEditor", "HudEditor", false).action();
    public final Setting<Integer> safeX = Setting.integer("safeX", "SafeX", 1, 0, 20);
    public final Setting<Integer> safeY = Setting.integer("safeY", "SafeY", 1, 0, 10);
    private final List<Setting<?>> settings = List.of(hudEditor, safeX, safeY);
    private final Map<String, HudElement> elements = new LinkedHashMap<>();
    private final EnumMap<HudAnchor, List<HudElement>> groups = new EnumMap<>(HudAnchor.class);
    private boolean enabled = true, initialized;
    private Keybind keybind;
    private JsonObject persisted = new JsonObject();
    private float panelX = 85, panelY = 10;
    private boolean panelExpanded = true;

    public HudManager() { for (HudAnchor anchor : HudAnchor.values()) groups.put(anchor, new ArrayList<>()); }
    public void initialize() {
        if (initialized) return;
        keybind = new Keybind("gui_hud", Keybind.UNBOUND);
        Myth.EVENT_MANAGER.register(this); initialized = true;
    }
    public Keybind getKeybind() { return keybind; }
    public List<Setting<?>> getSettings() { return settings; }
    public boolean isEnabled() { return enabled; }
    public void setEnabled(boolean enabled) { this.enabled = enabled; }
    public float getPanelX() { return panelX; }
    public float getPanelY() { return panelY; }
    public boolean isPanelExpanded() { return panelExpanded; }
    public void setPanel(float x, float y, boolean expanded) {
        if (!Float.isFinite(x) || !Float.isFinite(y)) throw new IllegalArgumentException("Invalid HUD panel position");
        panelX = x; panelY = y; panelExpanded = expanded;
    }
    public <T extends HudElement> T register(T element) {
        Objects.requireNonNull(element);
        if (elements.containsKey(element.getId())) throw new IllegalArgumentException("Duplicate HUD element: " + element.getId());
        ElementState saved = null;
        JsonObject stored = persisted.getAsJsonObject("elements");
        if (stored != null && stored.has(element.getId())) saved = decodeElement(element, stored.getAsJsonObject(element.getId()));
        elements.put(element.getId(), element); groups.get(element.getAnchor()).add(element);
        if (saved != null) applyElement(saved);
        return element;
    }
    public List<HudElement> getElements() { return List.copyOf(elements.values()); }
    public List<HudElement> getElements(HudAnchor anchor) { return List.copyOf(groups.get(anchor)); }
    public void setAnchor(HudElement element, HudAnchor anchor) {
        if (elements.get(element.getId()) != element) throw new IllegalArgumentException("HUD element is not registered");
        groups.get(element.getAnchor()).remove(element); element.anchor(anchor); groups.get(anchor).add(element);
    }
    public void swap(HudAnchor first, HudAnchor second) {
        List<HudElement> a = List.copyOf(groups.get(first)), b = List.copyOf(groups.get(second));
        for (HudElement element : a) setAnchor(element, second);
        for (HudElement element : b) setAnchor(element, first);
    }
    public boolean shouldRender(Minecraft client) {
        return enabled && client.level != null && !client.gui.hud.isHidden()
                && elements.values().stream().anyMatch(HudElement::isEnabled);
    }
    public void updateHeldKeybind() {
        if (initialized && keybind.canUpdateHeldState(enabled, Minecraft.getInstance().gui.screen() != null))
            enabled = keybind.getMode().heldState(keybind.isPressed());
    }
    @EventTarget private void onTick(GameTickEvent event) {
        if (event.getType() != EventType.POST || !initialized) return;
        Minecraft client = Minecraft.getInstance();
        if (client.isWindowActive() && client.gui.screen() == null && keybind.getMode() == Keybind.Mode.TOGGLE) {
            while (keybind.consumePress()) enabled = !enabled;
        } else keybind.discardPresses();
    }
    public void layout(FontManager fonts, float width, float height) {
        for (HudAnchor anchor : HudAnchor.values()) {
            float offset = safeY.get();
            List<HudElement> group = groups.get(anchor);
            for (HudElement element : group) {
                element.measure(fonts);
                float x = switch (anchor) {
                    case TOP_LEFT, BOTTOM_LEFT -> safeX.get();
                    case TOP_RIGHT, BOTTOM_RIGHT -> width - element.getWidth() - safeX.get();
                    case TOP_CENTER -> (width - element.getWidth()) / 2;
                    case NONE -> element.getRelativeX() * width;
                };
                float y = anchor == HudAnchor.NONE ? element.getRelativeY() * height
                        : anchor.isBottom() ? height - offset - element.getHeight() : offset;
                element.layout(x, y, width, height);
                if (element.isEnabled()) offset += element.getHeight();
            }
        }
    }
    public void render(FontManager fonts, float width, float height, float opacity, boolean editing) {
        if (!editing && !enabled) return;
        for (HudAnchor anchor : HudAnchor.values()) for (HudElement element : getElements(anchor)) {
            if (!element.isEnabled()) continue;
            element.render(new HudElement.RenderContext(ImGui.getBackgroundDrawList(), fonts, width, height,
                    element.getX(), element.getY(), element.getWidth(), element.getHeight(), opacity, editing));
        }
    }
    public JsonObject capture() {
        JsonObject root = persisted.deepCopy();
        root.addProperty("enabled", enabled); root.addProperty("key", keybind.serialize()); root.addProperty("mode", keybind.getMode().getId());
        JsonObject values = objectCopy(root, "settings");
        for (Setting<?> setting : settings) if (!setting.isAction()) values.add(setting.getId(), setting.toJson());
        root.add("settings", values);
        JsonObject panel = objectCopy(root, "panel");
        panel.addProperty("x", panelX); panel.addProperty("y", panelY); panel.addProperty("expanded", panelExpanded); root.add("panel", panel);
        JsonObject stored = objectCopy(root, "elements");
        for (HudElement element : elements.values()) {
            JsonObject data = objectCopy(stored, element.getId()); data.addProperty("enabled", element.isEnabled());
            JsonObject elementSettings = objectCopy(data, "settings");
            for (Setting<?> setting : element.getSettings()) if (!setting.isAction()) elementSettings.add(setting.getId(), setting.toJson());
            data.add("settings", elementSettings);
            JsonObject placement = objectCopy(data, "hud"); placement.addProperty("anchor", element.getAnchor().getId());
            placement.addProperty("index", groups.get(element.getAnchor()).indexOf(element));
            if (element.getAnchor() == HudAnchor.NONE) {
                placement.addProperty("x", element.getRelativeX()); placement.addProperty("y", element.getRelativeY());
            } else { placement.remove("x"); placement.remove("y"); }
            data.add("hud", placement); stored.add(element.getId(), data);
        }
        root.add("elements", stored); return root;
    }
    public State decode(JsonObject root) {
        JsonObject panel = root.has("panel") ? root.getAsJsonObject("panel") : new JsonObject();
        List<ElementState> states = new ArrayList<>();
        JsonObject stored = root.getAsJsonObject("elements");
        if (stored != null) for (HudElement element : elements.values()) if (stored.has(element.getId()))
            states.add(decodeElement(element, stored.getAsJsonObject(element.getId())));
        return new State(flag(root, "enabled", enabled), root.has("key") ? Keybind.parseSavedKey(root.get("key").getAsString()) : keybind.getBoundKey(),
                root.has("mode") ? Keybind.Mode.parse(root.get("mode").getAsString()) : keybind.getMode(), decodeSettings(root.getAsJsonObject("settings"), settings),
                coordinate(panel, "x", panelX, -100000, 100000), coordinate(panel, "y", panelY, -100000, 100000), flag(panel, "expanded", panelExpanded), List.copyOf(states));
    }
    public void apply(State state, JsonObject source) {
        enabled = state.enabled(); keybind.setKey(state.key()); keybind.setMode(state.mode());
        for (SettingState value : state.settings()) value.setting().apply(value.state());
        setPanel(state.panelX(), state.panelY(), state.panelExpanded());
        for (ElementState value : state.elements().stream().sorted(Comparator.comparingInt(ElementState::index)).toList()) applyElement(value);
        persisted = source.deepCopy();
    }
    private void applyElement(ElementState state) {
        HudElement element = state.element(); element.setEnabled(state.enabled()); element.setRelativePosition(state.x(), state.y());
        for (SettingState value : state.settings()) value.setting().apply(value.state());
        setAnchor(element, state.anchor());
        List<HudElement> group = groups.get(state.anchor()); group.remove(element); group.add(Math.min(state.index(), group.size()), element);
    }
    private ElementState decodeElement(HudElement element, JsonObject root) {
        JsonObject placement = root.has("hud") ? root.getAsJsonObject("hud") : new JsonObject();
        int index = placement.has("index") ? placement.get("index").getAsInt() : Math.max(0, groups.get(element.getAnchor()).indexOf(element));
        if (index < 0 || index > 10000) throw new IllegalArgumentException("Invalid HUD order");
        return new ElementState(element, flag(root, "enabled", element.isEnabled()), placement.has("anchor") ? HudAnchor.parse(placement.get("anchor").getAsString()) : element.getAnchor(), index,
                coordinate(placement, "x", element.getRelativeX(), 0, 1), coordinate(placement, "y", element.getRelativeY(), 0, 1), decodeSettings(root.getAsJsonObject("settings"), element.getSettings()));
    }
    private static List<SettingState> decodeSettings(JsonObject data, List<Setting<?>> settings) {
        List<SettingState> values = new ArrayList<>();
        if (data != null) for (Setting<?> setting : settings) if (!setting.isAction() && data.has(setting.getId()))
            values.add(new SettingState(setting, setting.decode(data.get(setting.getId()))));
        return List.copyOf(values);
    }
    private static JsonObject objectCopy(JsonObject root, String name) { return root.has(name) ? root.getAsJsonObject(name).deepCopy() : new JsonObject(); }
    private static boolean flag(JsonObject root, String name, boolean fallback) {
        if (!root.has(name)) return fallback;
        if (!root.get(name).isJsonPrimitive() || !root.getAsJsonPrimitive(name).isBoolean()) throw new IllegalArgumentException("Expected HUD boolean: " + name);
        return root.get(name).getAsBoolean();
    }
    private static float coordinate(JsonObject root, String name, float fallback, float min, float max) {
        if (!root.has(name)) return fallback;
        if (!root.get(name).isJsonPrimitive() || !root.getAsJsonPrimitive(name).isNumber()) throw new IllegalArgumentException("Expected HUD coordinate: " + name);
        float value = root.get(name).getAsFloat();
        if (!Float.isFinite(value) || value < min || value > max) throw new IllegalArgumentException("Invalid HUD coordinate: " + name);
        return value;
    }
}
