package today.starain.command;

import lombok.Getter;

@Getter
public class Command {
    private final String name;
    private final String[] aliases;

    public Command(String name, String... aliases) {
        this.name = name;
        this.aliases = aliases;
    }

    public void onCommand(String[] args) {
    }
}
