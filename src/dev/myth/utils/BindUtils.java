package dev.myth.utils;

import com.mojang.blaze3d.platform.InputConstants;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import org.lwjgl.glfw.GLFW;

public final class BindUtils {
    private static final Map<String, Integer> KEYS = Map.ofEntries(
            Map.entry("LSHIFT", GLFW.GLFW_KEY_LEFT_SHIFT), Map.entry("RSHIFT", GLFW.GLFW_KEY_RIGHT_SHIFT),
            Map.entry("LCTRL", GLFW.GLFW_KEY_LEFT_CONTROL), Map.entry("RCTRL", GLFW.GLFW_KEY_RIGHT_CONTROL),
            Map.entry("LALT", GLFW.GLFW_KEY_LEFT_ALT), Map.entry("RALT", GLFW.GLFW_KEY_RIGHT_ALT),
            Map.entry("SPACE", GLFW.GLFW_KEY_SPACE), Map.entry("ENTER", GLFW.GLFW_KEY_ENTER),
            Map.entry("ESC", GLFW.GLFW_KEY_ESCAPE), Map.entry("ESCAPE", GLFW.GLFW_KEY_ESCAPE),
            Map.entry("DELETE", GLFW.GLFW_KEY_DELETE), Map.entry("BACKSPACE", GLFW.GLFW_KEY_BACKSPACE),
            Map.entry("TAB", GLFW.GLFW_KEY_TAB), Map.entry("UP", GLFW.GLFW_KEY_UP),
            Map.entry("DOWN", GLFW.GLFW_KEY_DOWN), Map.entry("LEFT", GLFW.GLFW_KEY_LEFT),
            Map.entry("RIGHT", GLFW.GLFW_KEY_RIGHT), Map.entry("HOME", GLFW.GLFW_KEY_HOME),
            Map.entry("END", GLFW.GLFW_KEY_END), Map.entry("INSERT", GLFW.GLFW_KEY_INSERT),
            Map.entry("PAGEUP", GLFW.GLFW_KEY_PAGE_UP), Map.entry("PAGEDOWN", GLFW.GLFW_KEY_PAGE_DOWN));

    private BindUtils() {
    }

    public static InputConstants.Key parse(String input) {
        String key = input.toUpperCase(Locale.ROOT);
        if (List.of("NONE", "UNBIND", "NULL").contains(key)) {
            return InputConstants.UNKNOWN;
        }
        if (key.startsWith("KEY.KEYBOARD.") || key.startsWith("KEY.MOUSE.") || key.startsWith("SCANCODE.")) {
            return Keybind.parseSavedKey(input.toLowerCase(Locale.ROOT));
        }
        Integer code = KEYS.get(key);
        if (code == null && key.length() == 1 && Character.isLetterOrDigit(key.charAt(0)) && key.charAt(0) < 128) {
            code = (int) key.charAt(0);
        }
        if (code == null && key.matches("F([1-9]|1[0-9]|2[0-5])")) {
            code = GLFW.GLFW_KEY_F1 + Integer.parseInt(key.substring(1)) - 1;
        }
        if (code != null) {
            return InputConstants.Type.KEYSYM.getOrCreate(code);
        }
        int mouse = switch (key) {
            case "MOUSELEFT", "LMB" -> GLFW.GLFW_MOUSE_BUTTON_LEFT;
            case "MOUSERIGHT", "RMB" -> GLFW.GLFW_MOUSE_BUTTON_RIGHT;
            case "MOUSEMIDDLE", "MMB" -> GLFW.GLFW_MOUSE_BUTTON_MIDDLE;
            default -> -1;
        };
        if (mouse == -1 && key.matches("MOUSE[0-7]")) {
            mouse = Integer.parseInt(key.substring(5));
        }
        if (mouse >= 0) {
            return InputConstants.Type.MOUSE.getOrCreate(mouse);
        }
        if (key.matches("KEY[0-9]+")) {
            int numeric = Integer.parseInt(key.substring(3));
            if (numeric >= GLFW.GLFW_KEY_SPACE && numeric <= GLFW.GLFW_KEY_LAST) {
                return InputConstants.Type.KEYSYM.getOrCreate(numeric);
            }
        }
        throw new IllegalArgumentException("Unknown key: §f" + input);
    }

    public static String getBindName(InputConstants.Key key) {
        if (key.equals(InputConstants.UNKNOWN)) {
            return "None";
        }
        if (key.getType() == InputConstants.Type.MOUSE) {
            return switch (key.getValue()) {
                case GLFW.GLFW_MOUSE_BUTTON_LEFT -> "MouseLeft";
                case GLFW.GLFW_MOUSE_BUTTON_RIGHT -> "MouseRight";
                case GLFW.GLFW_MOUSE_BUTTON_MIDDLE -> "MouseMiddle";
                default -> "Mouse" + key.getValue();
            };
        }
        return key.getName().replace("key.keyboard.", "").replace('.', ' ').toUpperCase(Locale.ROOT);
    }

    public static List<String> suggestions() {
        var keys = new ArrayList<>(List.of("None", "MouseLeft", "MouseRight", "MouseMiddle", "Mouse3", "Mouse4"));
        keys.addAll(KEYS.keySet().stream().sorted().toList());
        for (char key = 'A'; key <= 'Z'; key++) {
            keys.add(String.valueOf(key));
        }
        for (char key = '0'; key <= '9'; key++) {
            keys.add(String.valueOf(key));
        }
        for (int key = 1; key <= 25; key++) {
            keys.add("F" + key);
        }
        return List.copyOf(keys);
    }
}
