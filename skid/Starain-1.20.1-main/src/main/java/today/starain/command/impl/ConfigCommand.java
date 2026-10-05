package today.starain.command.impl;

import today.starain.Starain;
import today.starain.command.Command;
import today.starain.utils.client.ChatUtils;

public final class ConfigCommand extends Command {
    private static final String USAGE = "usage: .config <load/save> <name>";

    public ConfigCommand() {
        super("config", "cfg");
    }

    @Override
    public void onCommand(String[] args) {
        if (args.length != 3) {
            ChatUtils.addChatMessage(USAGE);
            return;
        }

        String action = args[1];
        String configName = normalizeConfigName(args[2]);
        if (configName == null) {
            ChatUtils.addChatMessage("Invalid config name: ", args[2]);
            ChatUtils.addChatMessage("Config names can only contain letters, numbers, '_', '-' and '.'.");
            ChatUtils.addChatMessage(USAGE);
            return;
        }

        if ("save".equalsIgnoreCase(action)) {
            Starain.getInstance().getConfigManager().saveConfig(configName);
            ChatUtils.addChatMessage("Saved config: ", configName);
            return;
        }

        if ("load".equalsIgnoreCase(action)) {
            Starain.getInstance().getConfigManager().loadConfig(configName);
            ChatUtils.addChatMessage("Loaded config: ", configName);
            return;
        }

        ChatUtils.addChatMessage("Unknown action: ", action);
        ChatUtils.addChatMessage(USAGE);
    }

    private String normalizeConfigName(String input) {
        if (input == null) {
            return null;
        }

        String normalized = input.trim();
        if (normalized.isEmpty() || normalized.equals(".") || normalized.equals("..")) {
            return null;
        }

        for (int i = 0; i < normalized.length(); i++) {
            char c = normalized.charAt(i);
            if (!Character.isLetterOrDigit(c) && c != '_' && c != '-' && c != '.') {
                return null;
            }
        }
        return normalized;
    }
}
