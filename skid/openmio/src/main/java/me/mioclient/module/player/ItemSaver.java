package me.mioclient.module.player;

import me.mioclient.ArmorSearchHelper4;
import me.mioclient.BreakingProgressHelper;
import me.mioclient.FireworksHelper;
import me.mioclient.api.Category;
import me.mioclient.event.AttackBlockEvent;
import me.mioclient.event.Listen;
import me.mioclient.module.Module;
import net.minecraft.item.ArmorItem;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/player/ItemSaver.class */
public class ItemSaver extends Module {
    public ItemSaver() {
        super("ItemSaver", "Prevents your tools from breaking.", Category.PLAYER, new String[0]);
    }

    @Listen
    public void onAttackBlock(AttackBlockEvent attackBlockEvent) {
        int i = minecraftClient.player.getInventory().selectedSlot;
        if (is905(minecraftClient.player.getInventory().getStack(i)) || ((BreakingProgressHelper) minecraftClient.interactionManager).getCurrentBreakingBlock() == null || !minecraftClient.options.attackKey.isPressed()) {
            return;
        }
        FireworksHelper.do456(i == 8 ? i - 1 : i + 1);
        attackBlockEvent.do1162();
    }

    public static boolean is905(ItemStack itemStack) {
        return itemStack.isEmpty() || !itemStack.isDamageable() || (itemStack.getItem() instanceof ArmorItem) || itemStack.isOf(Items.ELYTRA) || ArmorSearchHelper4.get1905(itemStack) > 10;
    }
}
