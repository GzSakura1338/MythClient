package today.starain.value.impl;

import today.starain.module.ClientModule;
import today.starain.value.Value;

import java.util.function.Supplier;

public final class TextValue extends Value {
    private String value;

    public TextValue(String name, String value, ClientModule parent, Supplier<Boolean> visible) {
        super(name, parent, visible);
        this.value = value;
    }

    public TextValue(String name, String value, ClientModule parent) {
        this(name, value, parent, () -> true);
    }

    public String get() {
        return value;
    }

    public void set(String value) {
        this.value = value;
    }
}
