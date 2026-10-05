package me.mioclient;

import java.util.Iterator;
import java.util.List;
import net.minecraft.client.render.VertexConsumerProvider;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ShaderSearchHelper4.class */
public class ShaderSearchHelper4 implements SearchHelper_4 {
    public static ShaderFramebufferHelper shaderFramebufferHelper;
    public static ShaderFramebufferHelper shaderFramebufferHelper2;
    public static ShaderFramebufferHelper shaderFramebufferHelper3;
    public static ShaderFramebufferHelper shaderFramebufferHelper4;
    public static ShaderFramebufferHelper shaderFramebufferHelper5;
    public static List<ShaderFramebufferHelper> list;
    public static boolean flag;
    public static boolean flag2;

    public static void init() {
        shaderFramebufferHelper = new ShaderFramebufferHelper_6();
        shaderFramebufferHelper2 = new ShaderFramebufferHelper_2();
        shaderFramebufferHelper3 = new ShaderFramebufferHelper_5();
        shaderFramebufferHelper4 = new ShaderFramebufferHelper_4();
        shaderFramebufferHelper5 = new ShaderFramebufferHelper_3();
        list = List.of(shaderFramebufferHelper, shaderFramebufferHelper2, shaderFramebufferHelper3, shaderFramebufferHelper4, shaderFramebufferHelper5);
    }

    public static void do760() {
        Iterator<ShaderFramebufferHelper> it = list.iterator();
        while (it.hasNext()) {
            it.next().do760();
        }
    }

    public static void do866() {
        Iterator<ShaderFramebufferHelper> it = list.iterator();
        while (it.hasNext()) {
            it.next().do866();
        }
    }

    public static void do762(int i, int i2) {
        if (minecraftClient == null) {
            return;
        }
        Iterator<ShaderFramebufferHelper> it = list.iterator();
        while (it.hasNext()) {
            it.next().do762(i, i2);
        }
    }

    public static boolean is2855(VertexConsumerProvider vertexConsumerProvider) {
        Iterator<ShaderFramebufferHelper> it = list.iterator();
        while (it.hasNext()) {
            if (it.next().outlineVertexConsumerProvider == vertexConsumerProvider) {
                return true;
            }
        }
        return false;
    }
}
