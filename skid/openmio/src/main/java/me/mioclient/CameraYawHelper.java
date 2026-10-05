package me.mioclient;

import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/CameraYawHelper.class */
public interface CameraYawHelper {
    float getCameraYaw();

    float getCameraPitch();

    void setCameraYaw(float f);

    void setCameraPitch(float f);

    Vec3d mio$getPrevVelocity();
}
