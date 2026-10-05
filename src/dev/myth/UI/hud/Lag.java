package dev.myth.UI.hud;

import dev.myth.Myth;
import dev.myth.UI.font.FontManager;
import dev.myth.UI.setting.Setting;
import imgui.ImGui;
import net.minecraft.client.Minecraft;
import java.util.Locale;

public final class Lag extends HudElement {
    public final Setting<Integer> color = addSetting(Setting.color("color", "Color", 0xff808080, true, false));
    private String text;
    private float progress;
    public Lag() { super("lag", "Lag'O'Meter"); }
    @Override public String getDescription() { return "Warns when the multiplayer server stops sending packets."; }
    @Override protected float measureWidth(FontManager fonts) {
        long elapsed = Myth.HUD_MANAGER.data.silenceMillis();
        boolean visible = Myth.UI_MANAGER.isHudEditorOpen() || !Minecraft.getInstance().isLocalServer() && elapsed > 1000;
        text = visible ? String.format(Locale.ROOT, "The server is not responding for %.1fs", elapsed / 1000f) : null;
        float factor = 1 - (float) Math.pow(1 - Math.clamp(2 * ImGui.getIO().getDeltaTime(), 0, 1), 4);
        progress += ((visible ? 1 : 0) - progress) * factor;
        return text == null ? 0 : fonts.width(text);
    }
    @Override protected float measureHeight(FontManager fonts) { return text == null ? 0 : fonts.height() + 1; }
    @Override protected void render(RenderContext context) {
        if (text != null) context.fonts().draw(context.draw(), text, context.x(), context.y() - (1 - progress) * context.fonts().height(),
                color.color(HudDraw.theme()), context.opacity());
    }
}
