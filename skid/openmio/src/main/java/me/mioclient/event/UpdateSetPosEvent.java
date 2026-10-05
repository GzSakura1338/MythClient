package me.mioclient.event;

import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/UpdateSetPosEvent.class */
public class UpdateSetPosEvent extends Event {
    public double val;
    public double val2;
    public double val3;

    public UpdateSetPosEvent(double d, double d2, double d3) {
        this.val = d;
        this.val2 = d2;
        this.val3 = d3;
    }

    public void do1302(Vec3d vec3d) {
        this.val = vec3d.x;
        this.val2 = vec3d.y;
        this.val3 = vec3d.z;
    }

    public Vec3d getVec3d1303() {
        return new Vec3d(this.val, this.val2, this.val3);
    }
}
