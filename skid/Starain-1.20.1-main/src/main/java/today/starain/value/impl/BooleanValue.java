package today.starain.value.impl;

import today.starain.module.ClientModule;
import today.starain.value.Value;

import java.util.function.Supplier;

public final class BooleanValue extends Value {
    private boolean value;

    public BooleanValue(String name, boolean defaultValue, ClientModule parent, Supplier<Boolean> visible) {
        super(name, parent, visible);
        this.value = defaultValue;
    }

    public BooleanValue(String name, boolean defaultValue, ClientModule parent) {
        this(name, defaultValue, parent, () -> true);
    }

    public boolean get() {
        return value;
    }

    public void set(boolean value) {
        this.value = value;
    }

    public boolean toggle() {
        this.value = !this.value;
        return this.value;
    }
}
