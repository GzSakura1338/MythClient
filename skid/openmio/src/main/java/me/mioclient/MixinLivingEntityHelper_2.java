package me.mioclient;

import me.mioclient.mixin.ducks.DuckVec3d;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/MixinLivingEntityHelper_2.class */
public class MixinLivingEntityHelper_2 {
    public MixinLivingEntityHelper_2() {
        throw new AssertionError();
    }

    public static void do2581(Vec3d vec3d, double d, double d2, double d3) {
        do2582((DuckVec3d) vec3d, d, d2, d3);
    }

    public static void do2582(DuckVec3d duckVec3d, double d, double d2, double d3) {
        duckVec3d.setX(d);
        duckVec3d.setY(d2);
        duckVec3d.setZ(d3);
    }

    public static float get2583(Vec3d vec3d, Vec3d vec3d2) {
        return get2584(vec3d.x, vec3d.z, vec3d2.x, vec3d2.z);
    }

    public static float get2584(double d, double d2, double d3, double d4) {
        return (float) Math.hypot(d - d3, d2 - d4);
    }
}
