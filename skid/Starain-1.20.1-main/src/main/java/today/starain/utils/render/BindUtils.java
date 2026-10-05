package today.starain.utils.render;

import org.lwjgl.glfw.GLFW;

public final class BindUtils {
    public static final int MOUSE_OFFSET = 1000;

    private BindUtils() {
    }

    public static int keyboardCode(int glfwKey) {
        return glfwKey;
    }

    public static int mouseCode(int button) {
        return MOUSE_OFFSET + button;
    }

    public static boolean isMouseCode(int code) {
        return code >= MOUSE_OFFSET;
    }

    public static int extractMouseButton(int code) {
        return code - MOUSE_OFFSET;
    }

    public static String getBindName(int code) {
        if (code == GLFW.GLFW_KEY_UNKNOWN || code == 0) {
            return "None";
        }
        if (isMouseCode(code)) {
            return switch (extractMouseButton(code)) {
                case GLFW.GLFW_MOUSE_BUTTON_LEFT -> "MouseLeft";
                case GLFW.GLFW_MOUSE_BUTTON_RIGHT -> "MouseRight";
                case GLFW.GLFW_MOUSE_BUTTON_MIDDLE -> "MouseMiddle";
                default -> "Mouse" + extractMouseButton(code);
            };
        }
        String name = GLFW.glfwGetKeyName(code, 0);
        if (name != null && !name.isBlank()) {
            return name.toUpperCase();
        }
        return switch (code) {
            case GLFW.GLFW_KEY_LEFT_SHIFT -> "LShift";
            case GLFW.GLFW_KEY_RIGHT_SHIFT -> "RShift";
            case GLFW.GLFW_KEY_LEFT_CONTROL -> "LCtrl";
            case GLFW.GLFW_KEY_RIGHT_CONTROL -> "RCtrl";
            case GLFW.GLFW_KEY_LEFT_ALT -> "LAlt";
            case GLFW.GLFW_KEY_RIGHT_ALT -> "RAlt";
            case GLFW.GLFW_KEY_SPACE -> "Space";
            case GLFW.GLFW_KEY_ENTER -> "Enter";
            case GLFW.GLFW_KEY_ESCAPE -> "Esc";
            case GLFW.GLFW_KEY_DELETE -> "Delete";
            case GLFW.GLFW_KEY_BACKSPACE -> "Backspace";
            case GLFW.GLFW_KEY_TAB -> "Tab";
            default -> "Key" + code;
        };
    }
}
