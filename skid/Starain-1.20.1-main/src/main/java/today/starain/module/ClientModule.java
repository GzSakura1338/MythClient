package today.starain.module;

import lombok.Getter;
import lombok.Setter;
import today.starain.Starain;
import today.starain.module.impl.client.Notification;
import today.starain.utils.animation.SmoothAnimationTimer;
import today.starain.utils.wrapper.Wrapper;
import today.starain.value.Value;

import java.util.ArrayList;
import java.util.List;

@Setter
@Getter
public class ClientModule implements Wrapper {
    // ArrayList的动画
    public final SmoothAnimationTimer progress = new SmoothAnimationTimer(100, 0, 0.4f);
    private final List<Value> values = new ArrayList<>();
    private String name;
    private Category category;
    private int key;
    private boolean enabled;

    public ClientModule() {

    }

    protected void onEnable() {

    }

    protected void onDisable() {

    }

    public String getSuffix() {
        return null;
    }

    public void setEnabled(boolean enabled) {
        if (this.enabled == enabled) {
            return;
        }

        this.enabled = enabled;
        Notification.playToggleSound(enabled);
        if (enabled) {
            Starain.getInstance().getEventManager().register(this);
            onEnable();
        } else {
            Starain.getInstance().getEventManager().unregister(this);
            onDisable();
        }
    }

    public boolean toogle() {
        setEnabled(!enabled);
        return enabled;
    }

    public String getDisplayName() {
        String suffix = getSuffix();
        if (suffix == null || suffix.isEmpty()) {
            return name;
        } else {
            return name + " §7" + suffix;
        }
    }

}
