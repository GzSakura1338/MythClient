package today.starain.event.impl;

import lombok.Getter;
import lombok.Setter;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.phys.Vec3;
import today.starain.event.api.Cancellable;
import today.starain.event.api.Event;

@Getter
@Setter
public class StuckInBlockEvent implements Event, Cancellable {
    private boolean cancelled;
    private BlockState state;
    private Vec3 stuckSpeedMultiplier;

    public StuckInBlockEvent(BlockState state, Vec3 stuckSpeedMultiplier) {
        this.state = state;
        this.stuckSpeedMultiplier = stuckSpeedMultiplier;
    }
}
