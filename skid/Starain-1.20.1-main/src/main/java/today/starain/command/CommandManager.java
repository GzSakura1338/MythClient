package today.starain.command;

import com.mojang.brigadier.CommandDispatcher;
import com.mojang.brigadier.arguments.StringArgumentType;
import com.mojang.brigadier.builder.LiteralArgumentBuilder;
import com.mojang.brigadier.builder.RequiredArgumentBuilder;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import lombok.Getter;
import net.minecraft.commands.SharedSuggestionProvider;
import org.lwjgl.glfw.GLFW;
import today.starain.Starain;
import today.starain.command.impl.BindCommand;
import today.starain.command.impl.ConfigCommand;
import today.starain.command.impl.ToggleCommand;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.ChatEvent;
import today.starain.module.ClientModule;
import today.starain.utils.client.ChatUtils;
import today.starain.utils.wrapper.Wrapper;
import today.starain.value.Value;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

@Getter
public final class CommandManager {
    public static final String PREFIX = ".";

    private final List<Command> commands = new ArrayList<>();
    private final CommandDispatcher<SharedSuggestionProvider> dispatcher = new CommandDispatcher<>();

    private CommandManager() {
        load();
        loadDispatcher();
        Starain.getInstance().getEventManager().register(this);
    }

    public static CommandManager newInstance() {
        return new CommandManager();
    }

    private void load() {
        addCommand(new BindCommand(), new ToggleCommand(), new ConfigCommand());
    }

    private void loadDispatcher() {
        registerBindCommand("bind");
        registerBindCommand("b");
        registerToggleCommand("toggle");
        registerToggleCommand("t");
        registerConfigCommand("config");
        registerConfigCommand("cfg");
        for (ClientModule module : Starain.getInstance().getModuleManager().getModules()) {
            registerModuleCommand(module);
        }
    }

    private void registerModuleCommand(ClientModule module) {
        dispatcher.register(literal(module.getName())
                .then(RequiredArgumentBuilder
                        .<SharedSuggestionProvider, String>argument("value", StringArgumentType.word())
                        .suggests((context, builder) -> SharedSuggestionProvider.suggest(ValueEditor.valueNames(module), builder))
                        .then(RequiredArgumentBuilder
                                .<SharedSuggestionProvider, String>argument("input", StringArgumentType.greedyString())
                                .suggests((context, builder) -> SharedSuggestionProvider.suggest(inputSuggestions(module, builder), builder))
                                .executes(context -> 1))
                        .executes(context -> 1))
                .executes(context -> 1));
    }

    // "input" 该提示什么取决于前面选了哪个参数，而补全阶段拿不到已解析的参数，只能从原始输入里抠。
    private List<String> inputSuggestions(ClientModule module, SuggestionsBuilder builder) {
        String[] parts = builder.getInput().trim().split("\\s+");
        if (parts.length < 2) {
            return List.of();
        }
        Value value = ValueEditor.find(module, parts[1]);
        return value == null ? List.of() : ValueEditor.suggestionsFor(value);
    }

    private void addCommand(Command... commands) {
        for (Command command : commands) {
            this.commands.add(command);
            Starain.getInstance().getEventManager().register(command);
        }
    }

    private void registerBindCommand(String name) {
        dispatcher.register(literal(name)
                .then(moduleArgument()
                        .executes(context -> 1)
                        .then(RequiredArgumentBuilder
                                .<SharedSuggestionProvider, String>argument("key", StringArgumentType.word())
                                .suggests((context, builder) -> SharedSuggestionProvider.suggest(keySuggestions(), builder))
                                .executes(context -> 1)))
                .executes(context -> 1));
    }

    private void registerToggleCommand(String name) {
        dispatcher.register(literal(name)
                .then(moduleArgument().executes(context -> 1))
                .executes(context -> 1));
    }

    private void registerConfigCommand(String name) {
        dispatcher.register(literal(name)
                .then(RequiredArgumentBuilder
                        .<SharedSuggestionProvider, String>argument("action", StringArgumentType.word())
                        .suggests((context, builder) -> SharedSuggestionProvider.suggest(List.of("load", "save"), builder))
                        .then(RequiredArgumentBuilder
                                .<SharedSuggestionProvider, String>argument("name", StringArgumentType.word())
                                .suggests((context, builder) -> SharedSuggestionProvider.suggest(configSuggestions(), builder))
                                .executes(context -> 1))
                        .executes(context -> 1))
                .executes(context -> 1));
    }

    private List<String> configSuggestions() {
        List<String> suggestions = new ArrayList<>(List.of("latest"));
        File[] files = Wrapper.resolveConfigFolder().listFiles((dir, fileName) -> fileName.endsWith(".json"));
        if (files != null) {
            for (File file : files) {
                String fileName = file.getName();
                String configName = fileName.substring(0, fileName.length() - ".json".length());
                if (!suggestions.contains(configName)) {
                    suggestions.add(configName);
                }
            }
        }
        return suggestions;
    }

    private RequiredArgumentBuilder<SharedSuggestionProvider, String> moduleArgument() {
        return RequiredArgumentBuilder
                .<SharedSuggestionProvider, String>argument("module", StringArgumentType.word())
                .suggests((context, builder) -> SharedSuggestionProvider.suggest(
                        Starain.getInstance().getModuleManager().getModules().stream()
                                .map(module -> module.getName()),
                        builder
                ));
    }

    private LiteralArgumentBuilder<SharedSuggestionProvider> literal(String name) {
        return LiteralArgumentBuilder.literal(name);
    }

    private List<String> keySuggestions() {
        List<String> suggestions = new ArrayList<>(List.of(
                "None", "MouseLeft", "MouseRight", "MouseMiddle",
                "LShift", "RShift", "LCtrl", "RCtrl", "LAlt", "RAlt",
                "Space", "Enter", "Esc", "Delete", "Backspace", "Tab",
                "Up", "Down", "Left", "Right"
        ));

        for (int key = GLFW.GLFW_KEY_A; key <= GLFW.GLFW_KEY_Z; key++) {
            suggestions.add(String.valueOf((char) ('A' + key - GLFW.GLFW_KEY_A)));
        }
        for (int key = GLFW.GLFW_KEY_0; key <= GLFW.GLFW_KEY_9; key++) {
            suggestions.add(String.valueOf((char) ('0' + key - GLFW.GLFW_KEY_0)));
        }
        return suggestions;
    }

    @EventTarget
    public void onChat(ChatEvent event) {
        String message = event.getMessage();
        if (message == null || !message.startsWith(PREFIX)) {
            return;
        }

        String[] args = message.trim().split("\\s+");
        if (args.length == 0) {
            return;
        }

        String commandName = args[0].substring(PREFIX.length());
        for (Command command : commands) {
            if (command.getName().equalsIgnoreCase(commandName)) {
                command.onCommand(args);
                event.setCancelled(true);
                return;
            }

            for (String alias : command.getAliases()) {
                if (alias.equalsIgnoreCase(commandName)) {
                    command.onCommand(args);
                    event.setCancelled(true);
                    return;
                }
            }
        }

        // 没有对上任何命令，再试模块名：.<模块> <参数> <值>
        ClientModule module = Starain.getInstance().getModuleManager().getModule(commandName);
        if (module != null) {
            editModule(module, args);
            event.setCancelled(true);
        }
    }

    private void editModule(ClientModule module, String[] args) {
        if (args.length == 1) {
            ChatUtils.addChatMessage(module.getName(), " §7(", module.getCategory().getName(), ")");
            for (Value value : module.getValues()) {
                ChatUtils.addChatMessage(false, "  §7", ValueEditor.compactName(value), ": §f", ValueEditor.describe(value));
            }
            if (module.getValues().isEmpty()) {
                ChatUtils.addChatMessage(false, "  §7no values");
            }
            return;
        }

        Value value = ValueEditor.find(module, args[1]);
        if (value == null) {
            ChatUtils.addChatMessage("Unknown value: ", args[1]);
            return;
        }

        // 只给参数名不给值 -> 查询当前值
        if (args.length == 2) {
            ChatUtils.addChatMessage(module.getName(), " ", ValueEditor.compactName(value), " = ", ValueEditor.describe(value));
            return;
        }

        String result = ValueEditor.apply(value, args, 2);
        if (result == null) {
            ChatUtils.addChatMessage("Invalid value for ", ValueEditor.compactName(value), ": ", String.join(" ", java.util.Arrays.copyOfRange(args, 2, args.length)));
            List<String> hints = ValueEditor.suggestionsFor(value);
            if (!hints.isEmpty()) {
                ChatUtils.addChatMessage(false, "  §7expected: ", String.join(" / ", hints));
            }
            return;
        }

        ChatUtils.addChatMessage(module.getName(), " ", ValueEditor.compactName(value), " -> §f", result);
        Starain.getInstance().getConfigManager().saveConfig();
    }
}
