package me.mioclient;

import net.minecraft.client.MinecraftClient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/WelcomerHelper.class */
public final class WelcomerHelper {
    public static String username = null;
    public static int num = -1;
    public static int num2 = -1;

    public WelcomerHelper() {
        if (username == null) {
            username = MinecraftClient.getInstance().getSession().getUsername();
        }
    }

    public String getString2810() {
        return username;
    }

    public int get2811() {
        return num;
    }

    public boolean is2812() {
        return num2 == -1 || num2 == 4;
    }

    public boolean is2813() {
        return is2812() || num2 == 3;
    }
}
