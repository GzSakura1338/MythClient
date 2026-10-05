package me.mioclient.event;

import net.minecraft.block.BlockState;
import net.minecraft.client.render.VertexConsumer;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.util.math.BlockPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/DrawBlockOutlineEvent.class */
public class DrawBlockOutlineEvent extends Event {
    public MatrixStack matrixStack;
    public VertexConsumer vertexConsumer;
    public BlockPos blockPos;
    public BlockState blockState;

    public DrawBlockOutlineEvent(MatrixStack matrixStack, VertexConsumer vertexConsumer, BlockPos blockPos, BlockState blockState) {
        this.matrixStack = matrixStack;
        this.vertexConsumer = vertexConsumer;
        this.blockPos = blockPos;
        this.blockState = blockState;
    }

    public MatrixStack getMatrixStack1486() {
        return this.matrixStack;
    }

    public void do1487(MatrixStack matrixStack) {
        this.matrixStack = matrixStack;
    }

    public VertexConsumer getVertexConsumer1488() {
        return this.vertexConsumer;
    }

    public void do1489(VertexConsumer vertexConsumer) {
        this.vertexConsumer = vertexConsumer;
    }

    public BlockPos getBlockPos386() {
        return this.blockPos;
    }

    public void do667(BlockPos blockPos) {
        this.blockPos = blockPos;
    }

    public BlockState getBlockState670() {
        return this.blockState;
    }

    public void do668(BlockState blockState) {
        this.blockState = blockState;
    }
}
