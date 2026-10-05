package dev.myth.command;

import dev.myth.Myth;
import dev.myth.UI.setting.Setting;
import dev.myth.model.Category;
import dev.myth.model.Model;
import dev.myth.utils.ChatUtils;
import dev.myth.utils.ClientUtils;
import java.awt.Color;
import net.minecraft.network.chat.Component;

/** Mio's module toggle notifications, kept with local chat handling. */
public final class Notifications extends Model {
    public final Setting<Boolean> modules = addSetting(Setting.bool("modules", "Modules", true)
            .description("Shows module toggles in chat."));

    public Notifications() {
        super("notifications", "Notifications", Category.CLIENT);
        setDescription("Shows module toggle notifications in chat.");
    }
    @Override public boolean isDrawn() { return false; }

    public static void onToggle(Model model) {
        var notifications = Myth.MODEL_MANAGER.getModel(Notifications.class).orElse(null);
        if (notifications == null || !notifications.isEnabled() || !notifications.modules.get()
                || !ClientUtils.isInGame(ClientUtils.getClient())) return;

        Color color = model.isEnabled() ? new Color(0, 190, 50) : new Color(200, 0, 0);
        int brackets = color.darker().getRGB() & 0xffffff;
        var message = Component.empty()
                .append(Component.literal("[").withColor(brackets))
                .append(Component.literal(model.isEnabled() ? "+" : "-").withColor(color.getRGB() & 0xffffff))
                .append(Component.literal("] ").withColor(brackets))
                .append(Component.literal(model.getName()));
        ChatUtils.component(message);
    }
}
