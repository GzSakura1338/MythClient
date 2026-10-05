package me.mioclient;

import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ScaffoldMode.class */
public enum ScaffoldMode {
    PRE,
    POST;

    public ScaffoldData scaffoldData;
    public Vec3d vec3d;

    public ScaffoldMode getScaffoldMode874(ScaffoldData scaffoldData, Vec3d vec3d) {
        this.scaffoldData = scaffoldData;
        this.vec3d = vec3d;
        return this;
    }
}
