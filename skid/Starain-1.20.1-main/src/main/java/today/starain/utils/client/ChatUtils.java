package today.starain.utils.client;

import net.minecraft.client.gui.components.ChatComponent;
import net.minecraft.network.chat.Component;
import today.starain.Starain;
import today.starain.utils.wrapper.Wrapper;

public class ChatUtils implements Wrapper {
    private static final String PREFIX = "§7[§b" + Starain.NAME + "§7] ";

    public static void component(Component component) {
        if (mc.gui == null) {
            return;
        }
        ChatComponent chat = mc.gui.getChat();
        chat.addMessage(component);
    }

    public static void addChatMessage(String message) {
        addChatMessage(true, message);
    }

    public static void addChatMessage(String... message) {
        StringBuilder sb = new StringBuilder();
        for (String s : message) {
            sb.append(s);
        }
        addChatMessage(true, sb.toString());
    }


    public static void addChatMessage(boolean prefix, String... message) {
        StringBuilder sb = new StringBuilder();
        for (String s : message) {
            sb.append(s);
        }
        component(Component.nullToEmpty((prefix ? PREFIX : "") + sb));
    }
}
