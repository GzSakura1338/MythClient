package dev.myth.UI.hud;

import dev.myth.Myth;
import dev.myth.UI.font.FontManager;
import dev.myth.UI.setting.Setting;
import net.minecraft.client.Minecraft;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;

public final class Totems extends HudElement {
    public final Setting<Boolean> white = addSetting(Setting.bool("white", "White", false));
    public Totems() { super("totems", "Totems"); }
    @Override public String getDescription() { return "Shows the total number of totems in your inventory."; }
    @Override protected float measureWidth(FontManager fonts) { return 16; }
    @Override protected float measureHeight(FontManager fonts) { return 16; }
    @Override protected void render(RenderContext context) {
        var player = Minecraft.getInstance().player; if (player == null) return;
        int count = 0;
        for (int i = 0; i < player.getInventory().getContainerSize(); i++) {
            var item = player.getInventory().getItem(i); if (item.is(Items.TOTEM_OF_UNDYING)) count += item.getCount();
        }
        if (count == 0) return;
        Myth.HUD_MANAGER.assets.item(context.draw(), new ItemStack(Items.TOTEM_OF_UNDYING), context.x(), context.y(), 1, context.opacity());
        if (count > 1) {
            String text = Integer.toString(count);
            context.fonts().draw(context.draw(), text, context.x() + 17 - context.fonts().width(text), context.y() + 9,
                    white.get() ? 0xffffffff : Myth.HUD_MANAGER.colors.colorAt(context.y() + 9, context.fonts().height()), context.opacity());
        }
    }
}
