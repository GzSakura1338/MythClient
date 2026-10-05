package today.starain.command.impl;

import org.lwjgl.glfw.GLFW;
import today.starain.Starain;
import today.starain.command.Command;
import today.starain.module.ClientModule;
import today.starain.utils.client.ChatUtils;
import today.starain.utils.render.BindUtils;

public final class BindCommand extends Command {
    public BindCommand() {
        super("bind", "b");
    }

    @Override
    public void onCommand(String[] args) {
        if (args.length < 3) {
            ChatUtils.addChatMessage("usage: .bind <Module> <Key>");
            return;
        }

        ClientModule targetModule = Starain.getInstance().getModuleManager().getModule(args[1]);
        if (targetModule == null) {
            ChatUtils.addChatMessage("Unknown module: ", args[1]);
            return;
        }

        int bindCode = parseBindCode(args[2]);
        if (bindCode == Integer.MIN_VALUE) {
            ChatUtils.addChatMessage("Unknown key: ", args[2]);
            return;
        }

        targetModule.setKey(bindCode);
        if (bindCode == 0) {
            ChatUtils.addChatMessage("Module ", targetModule.getName(), " unbound");
        } else {
            ChatUtils.addChatMessage("Module ", targetModule.getName(), " bound to ", BindUtils.getBindName(bindCode));
        }
        Starain.getInstance().getConfigManager().saveConfig();
    }

    private int parseBindCode(String input) {
        if (input == null || input.isBlank()) {
            return Integer.MIN_VALUE;
        }

        String normalized = input.trim();
        String upper = normalized.toUpperCase();

        if (upper.equals("NONE") || upper.equals("UNBIND") || upper.equals("NULL")) {
            return 0;
        }

        return switch (upper) {
            case "MOUSELEFT", "LMB" -> BindUtils.mouseCode(GLFW.GLFW_MOUSE_BUTTON_LEFT);
            case "MOUSERIGHT", "RMB" -> BindUtils.mouseCode(GLFW.GLFW_MOUSE_BUTTON_RIGHT);
            case "MOUSEMIDDLE", "MMB" -> BindUtils.mouseCode(GLFW.GLFW_MOUSE_BUTTON_MIDDLE);
            case "LSHIFT" -> GLFW.GLFW_KEY_LEFT_SHIFT;
            case "RSHIFT" -> GLFW.GLFW_KEY_RIGHT_SHIFT;
            case "LCTRL", "LCONTROL" -> GLFW.GLFW_KEY_LEFT_CONTROL;
            case "RCTRL", "RCONTROL" -> GLFW.GLFW_KEY_RIGHT_CONTROL;
            case "LALT" -> GLFW.GLFW_KEY_LEFT_ALT;
            case "RALT" -> GLFW.GLFW_KEY_RIGHT_ALT;
            case "SPACE" -> GLFW.GLFW_KEY_SPACE;
            case "ENTER" -> GLFW.GLFW_KEY_ENTER;
            case "ESC", "ESCAPE" -> GLFW.GLFW_KEY_ESCAPE;
            case "DELETE", "DEL" -> GLFW.GLFW_KEY_DELETE;
            case "BACKSPACE", "BS" -> GLFW.GLFW_KEY_BACKSPACE;
            case "TAB" -> GLFW.GLFW_KEY_TAB;
            case "UP" -> GLFW.GLFW_KEY_UP;
            case "DOWN" -> GLFW.GLFW_KEY_DOWN;
            case "LEFT" -> GLFW.GLFW_KEY_LEFT;
            case "RIGHT" -> GLFW.GLFW_KEY_RIGHT;
            default -> parseDynamicKey(normalized, upper);
        };
    }

    private int parseDynamicKey(String normalized, String upper) {
        if (upper.startsWith("MOUSE") && upper.length() > 5) {
            try {
                return BindUtils.mouseCode(Integer.parseInt(upper.substring(5)));
            } catch (NumberFormatException ignored) {
                return Integer.MIN_VALUE;
            }
        }

        if (upper.startsWith("KEY") && upper.length() > 3) {
            try {
                return Integer.parseInt(upper.substring(3));
            } catch (NumberFormatException ignored) {
                return Integer.MIN_VALUE;
            }
        }

        int guessed = guessKeyFromName(upper);
        return guessed == GLFW.GLFW_KEY_UNKNOWN ? Integer.MIN_VALUE : guessed;
    }

    private int guessKeyFromName(String upper) {
        for (int key = GLFW.GLFW_KEY_SPACE; key <= GLFW.GLFW_KEY_LAST; key++) {
            String bindName = BindUtils.getBindName(key);
            if (bindName.equalsIgnoreCase(upper)) {
                return key;
            }
        }
        return GLFW.GLFW_KEY_UNKNOWN;
    }
}
