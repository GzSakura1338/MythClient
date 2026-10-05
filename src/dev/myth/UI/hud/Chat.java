package dev.myth.UI.hud;

import dev.myth.Myth;
import dev.myth.UI.font.FontManager;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.components.ChatComponent;

public final class Chat extends HudElement {
    public Chat() { super("chat", "Chat"); }
    @Override public String getDescription() { return "Repositions the vanilla chat, including clickable text."; }
    @Override protected float measureWidth(FontManager fonts) {
        var client = Minecraft.getInstance();
        return (float) ((client.options.chatWidth().get() * 280 + 40 + 12 * client.options.chatScale().get()) * nativeToHud());
    }
    @Override protected float measureHeight(FontManager fonts) {
        var client = Minecraft.getInstance();
        return (float) (ChatComponent.getHeight(client.options.chatHeightUnfocused().get()) * client.options.chatScale().get() * nativeToHud());
    }
    @Override protected void render(RenderContext context) {}
    private static float nativeToHud() {
        var client = Minecraft.getInstance();
        float logicalScale = client.getWindow().calculateScale(2, client.isEnforceUnicode()) * Myth.UI_MANAGER.getHudScale();
        return client.getWindow().getGuiScale() / logicalScale;
    }
    public float offsetX() { return getX() / nativeToHud(); }
    public float offsetY() { return (getY() + getHeight()) / nativeToHud() - (Minecraft.getInstance().getWindow().getGuiScaledHeight() - 40); }
    public static Chat active() {
        if (!Myth.HUD_MANAGER.isEnabled() || Minecraft.getInstance().level == null || Minecraft.getInstance().gui.hud.isHidden()) return null;
        return Myth.HUD_MANAGER.getElements().stream().filter(e -> e instanceof Chat && e.isEnabled()).map(e -> (Chat) e).findFirst().orElse(null);
    }
}
