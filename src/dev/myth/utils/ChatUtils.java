package dev.myth.utils;

import net.minecraft.network.chat.Component;

public final class ChatUtils {
    private static final String PREFIX = "§7[§bMyth§7] §f";

    private ChatUtils() {
    }

    public static void component(Component message) {
        var client = ClientUtils.getClient();
        client.execute(() -> client.gui.hud.getChat().addClientSystemMessage(message));
    }

    public static void addChatMessage(String... message) {
        addChatMessage(true, message);
    }

    public static void addChatMessage(boolean prefix, String... message) {
        component(Component.literal((prefix ? PREFIX : "") + String.join("", message)));
    }
}
