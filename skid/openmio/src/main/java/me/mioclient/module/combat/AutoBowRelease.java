package me.mioclient.module.combat;

import me.mioclient.AutoSignSearchHelper4;
import me.mioclient.FireworksHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.event.TickPostEvent;
import me.mioclient.module.Module;
import net.minecraft.item.ArrowItem;
import net.minecraft.item.CrossbowItem;
import net.minecraft.item.Item;
import net.minecraft.item.RangedWeaponItem;
import net.minecraft.network.packet.c2s.play.PlayerInteractItemC2SPacket;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/combat/AutoBowRelease.class */
public class AutoBowRelease extends Module {
    public Setting<Integer> delay;

    public AutoBowRelease() {
        super("AutoBowRelease", "Shoots your bow automatically.", Category.COMBAT, new String[0]);
        PhaseESPHelper.do1351(this);
    }

    @Listen
    public void onTickPost(TickPostEvent tickPostEvent) {
        if (is1469() || FireworksHelper.get449(itemStack -> {
            return itemStack.getItem() instanceof ArrowItem;
        }) <= 0) {
            return;
        }
        if (minecraftClient.player.getItemUseTime() >= this.delay.getValue().intValue()) {
            Item item = minecraftClient.player.getActiveItem().getItem();
            if (item instanceof RangedWeaponItem) {
                CrossbowItem crossbowItem = item instanceof CrossbowItem ? (CrossbowItem) item : null;
                if (minecraftClient.player.isUsingItem()) {
                    if (!(crossbowItem instanceof CrossbowItem) || crossbowItem.charged) {
                        minecraftClient.interactionManager.stopUsingItem(minecraftClient.player);
                        AutoSignSearchHelper4.do2571(new PlayerInteractItemC2SPacket(minecraftClient.player.getActiveHand(), AutoSignSearchHelper4.get2572(), minecraftClient.player.getYaw(), minecraftClient.player.getPitch()));
                    }
                }
            }
        }
    }
}
