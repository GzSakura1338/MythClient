package dev.myth.UI.hud;

import dev.myth.Myth;
import dev.myth.UI.font.FontManager;
import dev.myth.UI.setting.Setting;
import net.minecraft.client.Minecraft;
import net.minecraft.world.entity.EquipmentSlot;
import net.minecraft.world.entity.LivingEntity;

/** Mio Armor remains alongside the vanilla hotbar instead of the docking groups. */
public final class Armor extends HudElement {
    @Override public boolean isEditable() { return false; }
    public final Setting<Boolean> durability = addSetting(Setting.bool("durability", "Durability", true));
    public final Setting<Boolean> percentage = addSetting(Setting.bool("percentage", "Percentage", true));
    public final Setting<Boolean> barColor = addSetting(Setting.bool("barColor", "BarColor", true));
    private static final EquipmentSlot[] SLOTS = {EquipmentSlot.HEAD, EquipmentSlot.CHEST, EquipmentSlot.LEGS, EquipmentSlot.FEET};
    public Armor() { super("armor", "Armor"); }
    @Override public String getDescription() { return "Shows armor and durability next to the vanilla hotbar."; }
    @Override protected float measureWidth(FontManager fonts) { return 0; }
    @Override protected float measureHeight(FontManager fonts) { return 0; }
    @Override protected void render(RenderContext context) {
        var client = Minecraft.getInstance(); var player = client.player; if (player == null) return;
        float nativeScale = client.getWindow().getGuiScale() / imgui.ImGui.getIO().getDisplayFramebufferScaleY();
        int offset = 55 - (player.isCreative() ? 17 : 0);
        if ((player.isUnderWater() || player.getAirSupply() != player.getMaxAirSupply()) && player.getAirSupply() > 0 && !player.isCreative()) offset = 65;
        else if (player.getVehicle() instanceof LivingEntity mount) offset = 45 + (int) Math.ceil((mount.getMaxHealth() - 1) / 20) * 10;
        float x = context.screenWidth() / 2 + 15 * nativeScale, y = context.screenHeight() - offset * nativeScale;
        for (EquipmentSlot slot : SLOTS) {
            var item = player.getItemBySlot(slot); if (item.isEmpty()) continue;
            Myth.HUD_MANAGER.assets.decoratedItem(context, item, x, y, nativeScale, context.opacity());
            if (item.isDamageableItem() && durability.get()) {
                int remaining = Math.clamp(100 - (int) Math.ceil(item.getDamageValue() * 100f / item.getMaxDamage()), 1, 100);
                String text = remaining + (percentage.get() ? "%" : "");
                float textScale = (percentage.get() ? context.fonts().isEnabled() ? .7f : .65f : 1) * nativeScale;
                float textY = y - (3.8f + (percentage.get() ? 0 : 2)) * nativeScale;
                context.fonts().draw(context.draw(), text, x + 9 * nativeScale - context.fonts().width(text) * textScale / 2 - textScale,
                        textY, barColor.get() ? 0xff000000 | item.getBarColor() : Myth.HUD_MANAGER.colors.colorAt(textY, context.fonts().height()), context.opacity(), textScale);
            }
            x += 18 * nativeScale;
        }
    }
}
