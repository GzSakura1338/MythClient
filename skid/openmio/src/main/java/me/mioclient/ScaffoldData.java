package me.mioclient;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Direction;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.util.shape.VoxelShape;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ScaffoldData.class */
public final class ScaffoldData implements SearchHelper_4 {
    public final Direction direction;
    public final Vec3d vec3d;
    public final VoxelShape voxelShape;

    public ScaffoldData(Direction direction, Vec3d vec3d, VoxelShape voxelShape) {
        this.direction = direction;
        this.vec3d = vec3d;
        this.voxelShape = voxelShape;
    }

    public static ScaffoldData getScaffoldData1460(ScaffoldData_2 scaffoldData_2, boolean z) {
        Direction direction1462 = scaffoldData_2.getDirection1462();
        return new ScaffoldData(direction1462, z ? getVec3d1461(scaffoldData_2.getBlockPos12().offset(direction1462)) : scaffoldData_2.getBlockPos12().offset(direction1462).toCenterPos(), minecraftClient.world.getBlockState(scaffoldData_2.getBlockPos12().offset(direction1462)).getCollisionShape(minecraftClient.world, BlockPos.ORIGIN));
    }

    public static Vec3d getVec3d1461(BlockPos blockPos) {
        return new Vec3d(MathHelper.clamp(minecraftClient.player.getX(), blockPos.getX(), blockPos.getX() + 1), blockPos.getY(), MathHelper.clamp(minecraftClient.player.getZ(), blockPos.getZ(), blockPos.getZ() + 1));
    }




    public Direction getDirection1462() {
        return this.direction;
    }

    public Vec3d getVec3d843() {
        return this.vec3d;
    }

    public VoxelShape getVoxelShape1463() {
        return this.voxelShape;
    }
}
