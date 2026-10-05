package dev.myth.command.impl;

import dev.myth.Myth;
import dev.myth.command.Command;
import dev.myth.utils.ChatUtils;
import java.io.IOException;
import java.util.List;
import java.util.Locale;

public final class ConfigCommand extends Command {
    public ConfigCommand() {
        super("config", "cfg");
    }

    @Override
    public void onCommand(String[] args) throws IOException {
        if (args.length == 2 && args[1].equalsIgnoreCase("list")) {
            ChatUtils.addChatMessage("§7Current config: §b", Myth.CONFIG_MANAGER.getCurrentConfig());
            var configs = Myth.CONFIG_MANAGER.getConfigs();
            ChatUtils.addChatMessage("§7Available configs: §f", configs.isEmpty() ? "None" : String.join("§7, §f", configs));
            return;
        }
        if (args.length != 3) {
            String prefix = Myth.COMMAND_MANAGER.getPrefix();
            ChatUtils.addChatMessage("§7Usage: §b", prefix, "config §f<load/save> <name> §7or §b", prefix, "config list");
            return;
        }
        switch (args[1].toLowerCase(Locale.ROOT)) {
            case "save" -> {
                Myth.CONFIG_MANAGER.saveConfig(args[2]);
                ChatUtils.addChatMessage("§aSaved config: §b", Myth.CONFIG_MANAGER.getCurrentConfig());
            }
            case "load" -> {
                Myth.CONFIG_MANAGER.loadConfig(args[2]);
                ChatUtils.addChatMessage("§aLoaded config: §b", Myth.CONFIG_MANAGER.getCurrentConfig());
            }
            default -> throw new IllegalArgumentException("Unknown action: §f" + args[1] + "§c. Expected load or save.");
        }
    }

    @Override
    public List<String> getSuggestions(String[] args) throws IOException {
        if (args.length == 1) {
            return List.of("load", "save", "list");
        }
        return args.length == 2 && !args[0].equalsIgnoreCase("list")
                ? Myth.CONFIG_MANAGER.getConfigs() : List.of();
    }
}
