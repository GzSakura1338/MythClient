package me.mioclient;

import me.mioclient.event.Event;
import net.minecraft.block.BlockState;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.shape.VoxelShape;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/VoxelShapeEvent.class */
public class VoxelShapeEvent extends Event {
    public static final VoxelShapeEvent voxelShapeEvent = new VoxelShapeEvent();
    public VoxelShape voxelShape;
    public BlockPos blockPos;
    public BlockState blockState;

    public static VoxelShapeEvent getVoxelShapeEvent665(VoxelShape voxelShape, BlockPos blockPos, BlockState blockState) {
        voxelShapeEvent.do666(voxelShape);
        voxelShapeEvent.do667(blockPos);
        voxelShapeEvent.do668(blockState);
        return voxelShapeEvent;
    }

    public void do666(VoxelShape voxelShape) {
        this.voxelShape = voxelShape;
    }

    public void do667(BlockPos blockPos) {
        this.blockPos = blockPos;
    }

    public void do668(BlockState blockState) {
        this.blockState = blockState;
    }

    public VoxelShape getVoxelShape669() {
        return this.voxelShape;
    }

    public BlockPos getBlockPos386() {
        return this.blockPos;
    }

    public BlockState getBlockState670() {
        return this.blockState;
    }
}
