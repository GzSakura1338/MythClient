package me.mioclient;

import java.util.function.Supplier;
import net.minecraft.client.MinecraftClient;
import org.lwjgl.glfw.GLFW;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Mode_5.class */
public enum Mode_5 {
    STANDARD(() -> {
        return Long.valueOf(GLFW.glfwCreateStandardCursor(221185));
    }),
    POINTER(() -> {
        return Long.valueOf(GLFW.glfwCreateStandardCursor(221188));
    }),
    INPUT(() -> {
        return Long.valueOf(GLFW.glfwCreateStandardCursor(221186));
    });

    public final Supplier<Long> supplier;
    public long num = -1;
    public static Mode_5 mode_5 = STANDARD;

    Mode_5(Supplier supplier) {
        this.supplier = supplier;
    }

    public long get934() {
        if (this.num == -1) {
            this.num = this.supplier.get().longValue();
        }
        return this.num;
    }

    public void do935() {
        if (this == mode_5) {
            return;
        }
        GLFW.glfwSetCursor(MinecraftClient.getInstance().getWindow().getHandle(), get934());
        mode_5 = this;
    }
}
