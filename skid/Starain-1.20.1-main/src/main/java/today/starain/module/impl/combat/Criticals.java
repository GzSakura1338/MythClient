package today.starain.module.impl.combat;

import today.starain.event.api.EventType;
import today.starain.event.api.Priority;
import today.starain.event.api.annotations.EventPriority;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.GameTickEvent;
import today.starain.event.impl.MoveInputEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.utils.client.ChatUtils;

public class Criticals extends ClientModule {
    public static int fallTicks;
    public static boolean stopSprint;

    public Criticals() {
        setName("Criticals");
        setCategory(Category.COMBAT);
    }

    @EventPriority(Priority.HIGH)
    @EventTarget
    public void onGameTick(GameTickEvent event) {
        if (mc.player == null || event.getType() != EventType.PRE) return;
        if (mc.player.fallDistance <= 0 || mc.player.fallDistance >= 1) {
            fallTicks = 0;
        } else {
            fallTicks++;
            if (KillAura.targets != null && !KillAura.targets.isEmpty() && Velocity.attackQueue == 0) {
                stopSprint = true;
            }
        }
    }

    @EventTarget
    public void onMoveInput(MoveInputEvent event) {
        if (!stopSprint) return;
        stopSprint = false;
        event.setForward(0);
    }

    @Override
    public void onDisable() {
        fallTicks = 0;
        stopSprint = false;
    }

    @Override
    protected void onEnable() {
        ChatUtils.addChatMessage("我操为什么这个斯大林的刀爆和反击退打架");
    }
}
