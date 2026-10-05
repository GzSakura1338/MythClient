package me.mioclient;

import me.mioclient.event.Event;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Vec3dEvent.class */
public final class Vec3dEvent extends Event {
    public Vec3d vec3d;

    public Vec3dEvent(Vec3d vec3d) {
        this.vec3d = vec3d;
    }

    public Vec3d getVec3d1954() {
        return this.vec3d;
    }
}
