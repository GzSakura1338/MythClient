package dev.myth.utils;

import com.mojang.blaze3d.platform.InputConstants;
import dev.myth.Myth;
import dev.myth.event.api.annotations.EventTarget;
import dev.myth.event.impl.KeyboardEvent;
import dev.myth.event.impl.MouseButtonEvent;
import net.fabricmc.fabric.api.client.keymapping.v1.KeyMappingHelper;
import net.minecraft.client.KeyMapping;
import net.minecraft.client.Minecraft;
import net.minecraft.resources.Identifier;
import java.util.Locale;
import org.lwjgl.glfw.GLFW;

/** A registered binding, editable in Minecraft's controls screen. */
public final class Keybind {
    public enum Mode {
        TOGGLE("Toggle", "toggle"), HOLD("Hold", "hold"), HOLD_REVERSE("ReverseHold", "hold_reverse");
        private final String name;
        private final String id;
        Mode(String name, String id) { this.name = name; this.id = id; }
        public String getName() { return name; }
        public String getId() { return id; }
        public boolean heldState(boolean pressed) { return this == HOLD ? pressed : !pressed; }
        public static Mode parse(String value) {
            for (Mode mode : values()) if (mode.name().equalsIgnoreCase(value) || mode.id.equalsIgnoreCase(value) || mode.name.equalsIgnoreCase(value)) return mode;
            throw new IllegalArgumentException("Unknown keybind mode: " + value);
        }
    }
    public static final int UNBOUND = GLFW.GLFW_KEY_UNKNOWN;
    private static final KeyMapping.Category CATEGORY = KeyMapping.Category.register(
            Identifier.fromNamespaceAndPath(Myth.MOD_ID, "models"));

    private final KeyMapping mapping;
    private final boolean toggleOnly;
    private Mode mode = Mode.TOGGLE;
    private boolean physicalDown;

    public Keybind(String id, int defaultKey) {
        this(id, defaultKey, false);
    }

    public Keybind(String id, int defaultKey, boolean toggleOnly) {
        this.toggleOnly = toggleOnly;
        mapping = KeyMappingHelper.registerKeyMapping(new KeyMapping(
                "key." + Myth.MOD_ID + "." + id,
                InputConstants.Type.KEYSYM, defaultKey, CATEGORY));
        Myth.EVENT_MANAGER.register(this);
    }

    public KeyMapping getMapping() {
        return mapping;
    }

    public Mode getMode() { return mode; }
    public void setMode(Mode mode) { java.util.Objects.requireNonNull(mode); this.mode = toggleOnly ? Mode.TOGGLE : mode; discardPresses(); }
    public void cycleMode() { setMode(Mode.values()[(mode.ordinal() + 1) % Mode.values().length]); }
    public String getDisplayName() {
        InputConstants.Key key = getBoundKey();
        if (key.getValue() == UNBOUND) return "NONE";
        if (key.getType() == InputConstants.Type.MOUSE) return "MOUSE" + key.getValue();
        return key.getName().replace("key.keyboard.", "").replace('.', '_').toUpperCase(Locale.ROOT);
    }
    public String getDisplayLabel(boolean capturing) { return mode.getName() + " " + (capturing ? "..." : getDisplayName()); }
    public boolean canUpdateHeldState(boolean enabled, boolean screenOpen) {
        return getBoundKey().getValue() != UNBOUND && mode != Mode.TOGGLE
                && (!screenOpen || enabled == (mode == Mode.HOLD));
    }

    public InputConstants.Key getBoundKey() {
        return KeyMappingHelper.getBoundKeyOf(mapping);
    }

    public void setKey(InputConstants.Key key) {
        physicalDown = false;
        mapping.setDown(false);
        discardPresses();
        mapping.setKey(java.util.Objects.requireNonNull(key, "key"));
        KeyMapping.resetMapping();
    }

    public String serialize() {
        return mapping.saveString();
    }

    public static InputConstants.Key parseSavedKey(String saved) {
        if (saved == null || !(saved.startsWith("key.keyboard.")
                || saved.startsWith("key.mouse.") || saved.startsWith("scancode."))) {
            throw new IllegalArgumentException("Invalid key: " + saved);
        }
        return InputConstants.getKey(saved);
    }

    public boolean isPressed() {
        InputConstants.Key key = getBoundKey();
        if (key.getValue() == UNBOUND) return false;
        long window = Minecraft.getInstance().getWindow().handle();
        return switch (key.getType()) {
            case KEYSYM -> GLFW.glfwGetKey(window, key.getValue()) == GLFW.GLFW_PRESS;
            case MOUSE -> GLFW.glfwGetMouseButton(window, key.getValue()) == GLFW.GLFW_PRESS;
            case SCANCODE -> physicalDown;
        };
    }
    public void release() { physicalDown = false; mapping.setDown(false); discardPresses(); }

    @EventTarget private void onKeyboard(KeyboardEvent event) {
        InputConstants.Key key = getBoundKey();
        if (key.getType() == InputConstants.Type.KEYSYM && key.getValue() == event.key()
                || key.getType() == InputConstants.Type.SCANCODE && key.getValue() == event.scancode())
            physicalDown = event.action() != GLFW.GLFW_RELEASE;
    }
    @EventTarget private void onMouse(MouseButtonEvent event) {
        InputConstants.Key key = getBoundKey();
        if (key.getType() == InputConstants.Type.MOUSE && key.getValue() == event.button())
            physicalDown = event.action() != GLFW.GLFW_RELEASE;
    }

    public boolean consumePress() {
        return mapping.consumeClick();
    }

    public void discardPresses() {
        while (mapping.consumeClick()) {
            // Do not replay input from menus or an unfocused window.
        }
    }
}
