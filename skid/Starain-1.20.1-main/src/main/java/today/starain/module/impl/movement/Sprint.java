package today.starain.module.impl.movement;

import today.starain.event.api.EventType;
import today.starain.event.api.Priority;
import today.starain.event.api.annotations.EventPriority;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.MotionEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;

public class Sprint extends ClientModule {
    public Sprint() {
        setName("Sprint");
        setCategory(Category.MOVEMENT);
    }

    @EventTarget
    @EventPriority(Priority.HIGHEST)
    public void onMotion(MotionEvent e) {
        if (e.getType() == EventType.PRE) {
            mc.options.keySprint.setDown(true);
            mc.options.toggleSprint().set(false);
        }
    }

    @Override
    public void onDisable() {
        mc.options.keySprint.setDown(false);
    }
}
