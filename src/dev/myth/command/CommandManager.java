package dev.myth.command;

import com.mojang.brigadier.CommandDispatcher;
import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.arguments.StringArgumentType;
import com.mojang.brigadier.builder.LiteralArgumentBuilder;
import com.mojang.brigadier.builder.RequiredArgumentBuilder;
import com.mojang.brigadier.suggestion.Suggestions;
import dev.myth.Myth;
import dev.myth.command.impl.BindCommand;
import dev.myth.command.impl.ConfigCommand;
import dev.myth.command.impl.ToggleCommand;
import dev.myth.event.api.Priority;
import dev.myth.event.api.annotations.EventPriority;
import dev.myth.event.api.annotations.EventTarget;
import dev.myth.event.impl.ChatEvent;
import dev.myth.utils.ChatUtils;
import java.io.IOException;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import net.minecraft.client.multiplayer.ClientSuggestionProvider;

public final class CommandManager {
    public static final String PREFIX = ".";
    public static String getPrefix() { return Myth.UI_MANAGER.getCommandPrefix(); }
    private final Map<String, Command> commands = new LinkedHashMap<>();
    private final CommandDispatcher<ClientSuggestionProvider> dispatcher = new CommandDispatcher<>();
    private boolean initialized;

    public void initialize() {
        if (initialized) {
            return;
        }
        register(new BindCommand());
        register(new ToggleCommand());
        register(new ConfigCommand());
        Myth.EVENT_MANAGER.register(this);
        initialized = true;
    }

    public void register(Command command) {
        var names = new java.util.ArrayList<>(command.getAliases());
        names.addFirst(command.getName());
        for (String name : names) {
            if (!name.matches("[a-z][a-z0-9_-]*") || commands.containsKey(name)
                    || names.stream().filter(name::equals).count() > 1) {
                throw new IllegalArgumentException("Duplicate or invalid command name: " + name);
            }
        }
        for (String name : names) {
            commands.put(name, command);
            dispatcher.register(LiteralArgumentBuilder.<ClientSuggestionProvider>literal(name)
                    .then(RequiredArgumentBuilder.<ClientSuggestionProvider, String>argument("args", StringArgumentType.greedyString())
                            .suggests((context, builder) -> {
                                String raw = builder.getInput().substring(builder.getStart());
                                String[] args = raw.split("\\s+", -1);
                                var adjusted = builder.createOffset(builder.getStart() + raw.lastIndexOf(' ') + 1);
                                try {
                                    for (String suggestion : command.getSuggestions(args)) {
                                        if (suggestion.toLowerCase(Locale.ROOT).startsWith(adjusted.getRemainingLowerCase())) {
                                            adjusted.suggest(suggestion);
                                        }
                                    }
                                } catch (IOException exception) {
                                    Myth.LOGGER.warn("Could not list command suggestions", exception);
                                }
                                return adjusted.buildFuture();
                            })));
        }
    }

    public List<Command> getCommands() {
        return commands.values().stream().distinct().toList();
    }

    public CommandDispatcher<ClientSuggestionProvider> getDispatcher() {
        return dispatcher;
    }

    public CompletableFuture<Suggestions> getSuggestions(String text, int cursor, ClientSuggestionProvider source) {
        StringReader reader = new StringReader(text.toLowerCase(Locale.ROOT));
        reader.setCursor(getPrefix().length());
        return dispatcher.getCompletionSuggestions(dispatcher.parse(reader, source), cursor);
    }

    @EventTarget
    @EventPriority(Priority.HIGHEST)
    public void onChat(ChatEvent event) {
        String message = event.getMessage();
        if (event.isCancelled() || message == null || !message.startsWith(getPrefix())) {
            return;
        }
        // Cancel before parsing or execution so even invalid local commands stay local.
        event.setCancelled(true);
        String[] args = message.trim().split("\\s+");
        String name = args[0].substring(getPrefix().length()).toLowerCase(Locale.ROOT);
        Command command = commands.get(name);
        if (command == null) {
            if (!name.isEmpty()) {
                ChatUtils.addChatMessage("§cUnknown command: §f", name);
            }
            ChatUtils.addChatMessage("§7Commands: §b", getPrefix(), "bind §f<Module> <Key>§7, §b", getPrefix(), "toggle §f<Module>§7, §b", getPrefix(), "config §f<load/save> <name>§7, §b", getPrefix(), "config list");
            return;
        }
        try {
            command.onCommand(args);
        } catch (IllegalArgumentException exception) {
            ChatUtils.addChatMessage("§c", exception.getMessage());
        } catch (IOException | RuntimeException exception) {
            ChatUtils.addChatMessage(exception instanceof IOException
                    ? "§cFailed to read or write config. §7Check the log for details."
                    : "§cCommand failed. §7Check the log for details.");
            Myth.LOGGER.error("Command {} failed", name, exception);
        } finally {
            event.setCancelled(true);
        }
    }
}
