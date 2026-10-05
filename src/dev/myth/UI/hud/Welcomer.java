package dev.myth.UI.hud;

import dev.myth.UI.setting.Setting;
import java.util.List;
import net.minecraft.client.Minecraft;

/** Mio Welcomer Name/UID/Custom modes. */
public final class Welcomer extends TextHudElement {
    public final Setting<String> mode = addSetting(Setting.choice("mode", "Mode", "Name", List.of("Name", "UID", "Custom"))
            .description("UID uses Mio's uninitialized value (-1); Myth has no account UID."));
    public final Setting<String> text = addSetting(Setting.text("text", "Text", "Welcome to Myth.")
            .visibleWhen(() -> mode.get().equals("Custom")));

    public Welcomer() { super("welcomer", "Welcomer"); }
    @Override public String getDescription() { return "Draws a greeting or custom text."; }
    @Override protected String getText() {
        Minecraft client = Minecraft.getInstance();
        return switch (mode.get()) {
            case "UID" -> "Hello uid-1 :')"; // WelcomerHelper.num defaults to -1 in openmio.
            case "Custom" -> text.get();
            default -> "Hello " + (client.player == null ? client.getUser().getName() : client.player.getName().getString()) + " :')";
        };
    }
}
