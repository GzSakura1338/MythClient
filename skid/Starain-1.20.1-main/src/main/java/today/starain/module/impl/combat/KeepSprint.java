package today.starain.module.impl.combat;

import today.starain.event.api.EventType;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.AttackSlowdownEvent;
import today.starain.event.impl.MotionEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.utils.player.MoveUtils;


public class KeepSprint extends ClientModule {

    private boolean shouldReSprint = false;

    public KeepSprint() {
        setName("KeepSprint");
        setCategory(Category.COMBAT);
    }

    @EventTarget
    public void onAttackSlowdown(AttackSlowdownEvent event) {
        if (!MoveUtils.isMoving()) {
            return;
        }
        shouldReSprint = true;
    }

    @EventTarget
    public void onMotion(MotionEvent event) {
        if (event.getType() != EventType.POST) {
            return;
        }
        if (!shouldReSprint) {
            return;
        }
        if (!MoveUtils.isMoving()) {
            return;
        }
        if (!canReSprint()) {
            return;
        }
        shouldReSprint = false;
        mc.player.setSprinting(true);
    }


    private boolean canReSprint() {
        return mc.player.getFoodData().getFoodLevel() > 6
                && !mc.player.isBlocking();
    }

    @Override
    public void onDisable() {
        shouldReSprint = false;
    }
}
