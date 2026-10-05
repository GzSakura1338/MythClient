package today.starain.value.impl;

import lombok.Getter;
import today.starain.module.ClientModule;
import today.starain.value.Value;

import java.util.Arrays;
import java.util.function.Supplier;

public final class ModeValue extends Value {
    @Getter
    private final String[] modes;
    private String mode;

    public ModeValue(String name, String mode, String[] modes, ClientModule parent, Supplier<Boolean> visible) {
        super(name, parent, visible);
        this.mode = mode;
        this.modes = modes;
    }

    public ModeValue(String name, String mode, String[] modes, ClientModule parent) {
        this(name, mode, modes, parent, () -> true);
    }

    public String get() {
        return mode;
    }

    public void set(String mode) {
        if (Arrays.stream(modes).noneMatch(candidate -> candidate.equalsIgnoreCase(mode))) {
            return;
        }
        this.mode = mode;
    }

    public boolean isMode(String mode) {
        return this.mode.equalsIgnoreCase(mode);
    }

    public void next() {
        for (int index = 0; index < modes.length; index++) {
            if (modes[index].equalsIgnoreCase(mode)) {
                mode = modes[(index + 1) % modes.length];
                return;
            }
        }
        mode = modes.length == 0 ? "" : modes[0];
    }
}
