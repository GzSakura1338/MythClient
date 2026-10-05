package today.starain.command.impl;

import today.starain.Starain;
import today.starain.command.Command;
import today.starain.module.ClientModule;
import today.starain.utils.client.ChatUtils;

public final class ToggleCommand extends Command {
    public ToggleCommand() {
        super("toggle", "t");
    }

    @Override
    public void onCommand(String[] args) {
        if (args.length < 2) {
            ChatUtils.addChatMessage("usage: .toggle <Module>");
            return;
        }

        ClientModule module = Starain.getInstance().getModuleManager().getModule(args[1]);
        if (module == null) {
            ChatUtils.addChatMessage("Unknown module: ", args[1]);
            return;
        }

        boolean enabled = module.toogle();
        ChatUtils.addChatMessage("Module ", module.getName(), enabled ? " enabled" : " disabled");
    }
}
