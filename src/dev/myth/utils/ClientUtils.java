package dev.myth.utils;

import net.minecraft.client.Minecraft;

public final class ClientUtils {
    private ClientUtils() {
    }

    public static Minecraft getClient() {
        return Minecraft.getInstance();
    }

    public static boolean isInGame(Minecraft client) {
        return client.player != null && client.level != null;
    }

    public static boolean canHandleKeybinds(Minecraft client) {
        return client.gui.screen() == null && client.gui.overlay() == null && client.isWindowActive();
    }
}
