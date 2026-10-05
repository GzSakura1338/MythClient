package dev.myth.command.impl;

import dev.myth.Myth;
import dev.myth.command.Command;
import dev.myth.model.Model;
import dev.myth.utils.ChatUtils;
import java.io.IOException;
import java.util.List;

public final class ToggleCommand extends Command {
    public ToggleCommand() {
        super("toggle", "t");
    }

    static Model findModel(String name) {
        return Myth.MODEL_MANAGER.getModels().stream().filter(model -> model.getId().equalsIgnoreCase(name)
                || model.getName().equalsIgnoreCase(name)).findFirst()
                .orElseThrow(() -> new IllegalArgumentException("Unknown module: §f" + name));
    }

    static List<String> modelSuggestions() {
        return Myth.MODEL_MANAGER.getModels().stream().map(Model::getId).toList();
    }

    @Override
    public void onCommand(String[] args) throws IOException {
        if (args.length != 2) {
            ChatUtils.addChatMessage("§7Usage: §b", Myth.COMMAND_MANAGER.getPrefix(), "toggle §f<Module>");
            return;
        }
        Model model = findModel(args[1]);
        model.toggle();
        Myth.CONFIG_MANAGER.saveConfig();
    }

    @Override
    public List<String> getSuggestions(String[] args) {
        return args.length == 1 ? modelSuggestions() : List.of();
    }
}
