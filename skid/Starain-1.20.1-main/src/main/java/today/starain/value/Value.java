package today.starain.value;

import lombok.Getter;
import lombok.Setter;
import today.starain.module.ClientModule;

import java.util.function.Supplier;

@Getter
@Setter
public class Value {
    private final String name;
    private final Supplier<Boolean> visible;
    private final ClientModule parent;

    public Value(String name, ClientModule parent, Supplier<Boolean> visible) {
        this.name = name;
        this.parent = parent;
        this.visible = visible;
        if (parent != null) {
            parent.getValues().add(this);
        }
    }

    public boolean isVisible() {
        return visible == null || visible.get();
    }
}
