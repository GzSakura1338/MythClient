package me.mioclient.event;

import me.mioclient.MixinLivingEntityHelper_2;
import net.minecraft.entity.MovementType;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/MoveEvent.class */
public class MoveEvent extends Event {
    public final Vec3d vec3d;
    public final Vec3d vec3d2;
    public MovementType movementType;

    public MoveEvent(Vec3d vec3d, MovementType movementType) {
        this.vec3d = new Vec3d(vec3d.x, vec3d.y, vec3d.z);
        this.vec3d2 = vec3d;
        this.movementType = movementType;
    }

    public Vec3d getVec3d689() {
        return this.vec3d2;
    }

    public void do690(Vec3d vec3d) {
        MixinLivingEntityHelper_2.do2581(this.vec3d2, vec3d.x, vec3d.y, vec3d.z);
    }

    public void do691(double d, double d2) {
        do690(new Vec3d(d, get692(), d2));
    }

    public double get515() {
        return this.vec3d2.x;
    }

    public double get692() {
        return this.vec3d2.y;
    }

    public void setY(double d) {
        do690(new Vec3d(get515(), d, get516()));
    }

    public double get516() {
        return this.vec3d2.z;
    }

    public MovementType getMovementType693() {
        return this.movementType;
    }

    public void do694(MovementType movementType) {
        this.movementType = movementType;
    }

    public Vec3d getVec3d695() {
        return this.vec3d;
    }
}
