package dev.myth.command;

import java.io.IOException;
import java.util.List;
import java.util.Objects;

public abstract class Command {
    private final String name;
    private final List<String> aliases;

    protected Command(String name, String... aliases) {
        this.name = Objects.requireNonNull(name, "name");
        this.aliases = List.of(aliases);
    }

    public final String getName() {
        return name;
    }

    public final List<String> getAliases() {
        return aliases;
    }

    /** args[0] includes the prefix, matching Starain's command convention. */
    public abstract void onCommand(String[] args) throws IOException;

    public List<String> getSuggestions(String[] args) throws IOException {
        return List.of();
    }
}
