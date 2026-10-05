package me.mioclient;

import net.minecraft.block.BlockState;
import net.minecraft.block.ShapeContext;
import net.minecraft.client.MinecraftClient;
import net.minecraft.entity.Entity;
import net.minecraft.fluid.FluidState;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Vec3d;
import net.minecraft.util.shape.VoxelShape;
import net.minecraft.util.shape.VoxelShapes;
import net.minecraft.world.BlockView;
import net.minecraft.world.RaycastContext;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/HoleSnapHelper_2.class */
public final class HoleSnapHelper_2 {
    public final Vec3d vec3d;
    public final Vec3d vec3d2;
    public final RaycastContext.ShapeType shapeType;
    public final RaycastContext.FluidHandling fluidHandling;
    public final ShapeContext shapeContext;
    public final HoleSnapHelper holeSnapHelper;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/HoleSnapHelper_2$Inner.class */
    public static class Inner {
        public final Vec3d vec3d;
        public final Vec3d vec3d2;
        public RaycastContext.ShapeType shapeType = RaycastContext.ShapeType.COLLIDER;
        public RaycastContext.FluidHandling fluidHandling = RaycastContext.FluidHandling.NONE;
        public Entity entity = MinecraftClient.getInstance().player;
        public HoleSnapHelper holeSnapHelper = HoleSnapHelper.holeSnapHelper;

        public Inner(Vec3d vec3d, Vec3d vec3d2) {
            this.vec3d = vec3d;
            this.vec3d2 = vec3d2;
        }

        public Inner getInner1601(RaycastContext.ShapeType shapeType) {
            this.shapeType = shapeType;
            return this;
        }

        public Inner getInner1602(RaycastContext.FluidHandling fluidHandling) {
            this.fluidHandling = fluidHandling;
            return this;
        }

        public Inner getInner1603(Entity entity) {
            this.entity = entity;
            return this;
        }

        public Inner getInner1604(HoleSnapHelper holeSnapHelper) {
            this.holeSnapHelper = holeSnapHelper;
            return this;
        }

        public Inner getInner1605(HoleSnapHelper... holeSnapHelperArr) {
            this.holeSnapHelper = HoleSnapHelper.getHoleSnapHelper1678(holeSnapHelperArr);
            return this;
        }

        public HoleSnapHelper_2 getHoleSnapHelper_21606() {
            return new HoleSnapHelper_2(this.vec3d, this.vec3d2, this.shapeType, this.fluidHandling, this.entity, this.holeSnapHelper);
        }
    }

    public HoleSnapHelper_2(Vec3d vec3d, Vec3d vec3d2, RaycastContext.ShapeType shapeType, RaycastContext.FluidHandling fluidHandling, Entity entity, HoleSnapHelper holeSnapHelper) {
        this.vec3d = vec3d;
        this.vec3d2 = vec3d2;
        this.shapeType = shapeType;
        this.fluidHandling = fluidHandling;
        this.shapeContext = ShapeContext.of(entity);
        this.holeSnapHelper = holeSnapHelper;
    }

    public Vec3d getVec3d2218() {
        return this.vec3d;
    }

    public Vec3d getVec3d2219() {
        return this.vec3d2;
    }

    public RaycastContext.ShapeType getShapeType2220() {
        return this.shapeType;
    }

    public RaycastContext.FluidHandling getFluidHandling2221() {
        return this.fluidHandling;
    }

    public ShapeContext getShapeContext2222() {
        return this.shapeContext;
    }

    public HoleSnapHelper getHoleSnapHelper2223() {
        return this.holeSnapHelper;
    }

    public VoxelShape getVoxelShape2224(BlockState blockState, BlockView blockView, BlockPos blockPos) {
        return this.shapeType.get(blockState, blockView, blockPos, this.shapeContext);
    }

    public VoxelShape getVoxelShape2225(FluidState fluidState, BlockView blockView, BlockPos blockPos) {
        return this.fluidHandling.handled(fluidState) ? fluidState.getShape(blockView, blockPos) : VoxelShapes.empty();
    }
}
