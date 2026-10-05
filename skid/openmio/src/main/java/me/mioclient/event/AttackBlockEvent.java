package me.mioclient.event;

import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Direction;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/AttackBlockEvent.class */
public final class AttackBlockEvent extends Event {
    public final BlockPos blockPos;
    public final Direction direction;

    public AttackBlockEvent(BlockPos blockPos, Direction direction) {
        this.blockPos = blockPos;
        this.direction = direction;
    }

    public BlockPos getBlockPos386() {
        return this.blockPos;
    }

    public Direction getDirection387() {
        return this.direction;
    }
}
