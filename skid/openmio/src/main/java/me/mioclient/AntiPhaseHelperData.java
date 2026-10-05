package me.mioclient;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Direction;
import net.minecraft.util.shape.VoxelShape;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AntiPhaseHelperData.class */
public final class AntiPhaseHelperData {
    public final Direction direction;
    public final Box box;

    public AntiPhaseHelperData(Direction direction, Box box) {
        this.direction = direction;
        this.box = box;
    }

    public static AntiPhaseHelperData getAntiPhaseHelperData1239(Direction direction, VoxelShape voxelShape) {
        return new AntiPhaseHelperData(direction.getOpposite(), voxelShape.getBoundingBox());
    }




    public Direction getDirection842() {
        return this.direction;
    }

    public Box getBox799() {
        return this.box;
    }
}
