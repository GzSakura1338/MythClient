package dev.myth.UI.hud;

import dev.myth.Myth;
import dev.myth.UI.font.FontManager;
import dev.myth.UI.setting.Setting;
import imgui.ImGui;
import net.minecraft.client.Minecraft;

public final class Inventory extends HudElement {
    public final Setting<Integer> background = addSetting(Setting.color("background", "Background", 0x320a0a0a, true, false));
    public final Setting<Integer> outline = addSetting(Setting.color("outline", "Outline", 0x640a0a0a, true, false));
    public final Setting<Boolean> hideEmpty = addSetting(Setting.bool("hideEmpty", "HideEmpty", true));
    public final Setting<Float> size = addSetting(Setting.decimal("size", "Size", 1, .5f, 2));
    private float fade;
    public Inventory() { super("inventory", "Inventory"); }
    @Override public String getDescription() { return "Shows the 27 inventory slots above the hotbar."; }
    @Override protected float measureWidth(FontManager fonts) { return 162 * size.get(); }
    @Override protected float measureHeight(FontManager fonts) { return 54 * size.get(); }
    @Override protected void render(RenderContext context) {
        var client = Minecraft.getInstance(); boolean empty = true;
        if (client.player != null) for (int i = 9; i < 36; i++) if (!client.player.getInventory().getItem(i).isEmpty()) { empty = false; break; }
        float target = !hideEmpty.get() || !empty ? 1 : 0;
        fade += (target - fade) * Math.clamp(ImGui.getIO().getDeltaTime() / .2f, 0, 1);
        float alpha = fade * context.opacity(), s = size.get();
        HudDraw.rect(context.draw(), context.x(), context.y(), context.width(), context.height(), background.color(HudDraw.theme()), alpha);
        HudDraw.outline(context.draw(), context.x() - s, context.y() - s, context.width(), context.height(), outline.color(HudDraw.theme()), alpha);
        if (client.player == null) return;
        // Items share the panel's HideEmpty fade; drawing them at full opacity would pop.
        for (int i = 0; i < 27; i++) Myth.HUD_MANAGER.assets.decoratedItem(context, client.player.getInventory().getItem(i + 9),
                context.x() + i % 9 * 18 * s, context.y() + i / 9 * 18 * s, s, alpha);
    }
}
