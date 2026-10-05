package dev.myth.event.impl;

import dev.myth.event.api.CancellableEvent;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.phys.Vec3;

public class StuckInBlockEvent extends CancellableEvent {
    private BlockState state;
    private Vec3 stuckSpeedMultiplier;

    public StuckInBlockEvent(BlockState state, Vec3 stuckSpeedMultiplier) {
        this.state = state;
        this.stuckSpeedMultiplier = stuckSpeedMultiplier;
    }

    public BlockState getState() {
        return state;
    }

    public void setState(BlockState state) {
        this.state = state;
    }

    public Vec3 getStuckSpeedMultiplier() {
        return stuckSpeedMultiplier;
    }

    public void setStuckSpeedMultiplier(Vec3 stuckSpeedMultiplier) {
        this.stuckSpeedMultiplier = stuckSpeedMultiplier;
    }
}
