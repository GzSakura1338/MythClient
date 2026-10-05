package dev.myth.command.impl;

import dev.myth.Myth;
import dev.myth.command.Command;
import dev.myth.utils.BindUtils;
import dev.myth.utils.ChatUtils;
import java.io.IOException;
import java.util.List;

public final class BindCommand extends Command {
    public BindCommand() {
        super("bind", "b");
    }

    @Override
    public void onCommand(String[] args) throws IOException {
        if (args.length != 3) {
            ChatUtils.addChatMessage("§7Usage: §b", Myth.COMMAND_MANAGER.getPrefix(), "bind §f<Module> <Key|None>");
            return;
        }
        var model = ToggleCommand.findModel(args[1]);
        var key = BindUtils.parse(args[2]);
        var binding = Myth.MODEL_MANAGER.getKeybind(model.getId()).orElseThrow();
        binding.setKey(key);
        Myth.CONFIG_MANAGER.saveConfig();
        if (key.equals(com.mojang.blaze3d.platform.InputConstants.UNKNOWN)) {
            ChatUtils.addChatMessage("§7Module §b", model.getName(), " §7unbound");
        } else {
            ChatUtils.addChatMessage("§7Module §b", model.getName(), " §7bound to §e", BindUtils.getBindName(key));
        }
    }

    @Override
    public List<String> getSuggestions(String[] args) {
        return switch (args.length) {
            case 1 -> ToggleCommand.modelSuggestions();
            case 2 -> BindUtils.suggestions();
            default -> List.of();
        };
    }
}
