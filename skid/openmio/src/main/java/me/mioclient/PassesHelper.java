package me.mioclient;

import java.util.List;
import net.minecraft.client.gl.PostEffectPass;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/PassesHelper.class */
public interface PassesHelper {
    void addFakeTarget(String str, net.minecraft.client.gl.Framebuffer framebuffer);

    List<PostEffectPass> getPasses();
}
