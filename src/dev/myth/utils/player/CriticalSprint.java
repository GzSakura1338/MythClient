package dev.myth.utils.player;

import dev.myth.event.impl.MoveInputEvent;

/** NovaClient CriticalSprint.h: consume one forward-input interruption. */
public final class CriticalSprint {
    private boolean pending;
    public void reset() { pending = false; }
    public void tick(double fallDistance, boolean hasTargets, int attackQueue) {
        pending = fallDistance > 0 && fallDistance < 1 && hasTargets && attackQueue == 0;
    }
    public void input(MoveInputEvent event) {
        if (!pending) return;
        pending = false;
        event.setForward(0);
    }
}
