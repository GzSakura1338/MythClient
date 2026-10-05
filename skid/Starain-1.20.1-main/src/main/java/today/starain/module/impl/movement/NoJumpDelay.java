package today.starain.module.impl.movement;

import today.starain.event.api.EventType;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.MotionEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;

public class NoJumpDelay extends ClientModule {

    public NoJumpDelay() {
        setName("NoJumpDelay");
        setCategory(Category.MOVEMENT);
    }

    @EventTarget
    public void onMotion(MotionEvent e) {
        if (e.getType() == EventType.PRE) {
            mc.player.noJumpDelay = 0;
        }
    }
}
