package me.mioclient;

import java.util.Iterator;
import me.mioclient.module.combat.Offhand;
import me.mioclient.module.exploit.XCarry;
import net.minecraft.client.gui.screen.ingame.GenericContainerScreen;
import net.minecraft.client.gui.screen.ingame.ShulkerBoxScreen;
import net.minecraft.item.Item;
import net.minecraft.item.Items;
import net.minecraft.screen.slot.Slot;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/OffhandMode.class */
public enum OffhandMode implements SearchHelper_4, EnumSettingHelper {
    Totem("Totem", Items.TOTEM_OF_UNDYING),
    Crystal("Crystal", Items.END_CRYSTAL),
    Gapple("Gapple", Items.GOLDEN_APPLE) { // from class: me.mioclient.OffhandMode.Inner
        @Override // me.mioclient.OffhandMode
        public Item getItem1241(boolean z) {
            return z ? super.getItem1241(true) : Items.ENCHANTED_GOLDEN_APPLE;
        }
    },
    Custom("Custom", Items.SHIELD) { // from class: me.mioclient.OffhandMode.Inner_2
        @Override // me.mioclient.OffhandMode
        public Item getItem1241(boolean z) {
            return OffhandMode.offhand.custom.getValue().stream().findAny().orElse(null);
        }
    };

    public static final XCarry xCarry = (XCarry) BaritoneHelper_3.baritoneHelper_4.getModule117(XCarry.class);
    public static final Offhand offhand = (Offhand) BaritoneHelper_3.baritoneHelper_4.getModule117(Offhand.class);
    public final String name;
    public final Item item;

    OffhandMode(String str, Item item) {
        this.name = str;
        this.item = item;
    }

    public int get1240(boolean z) {
        Item item1241 = getItem1241(z);
        if (item1241 == null || minecraftClient.player.getOffHandStack().isOf(item1241)) {
            return -1;
        }
        if ((minecraftClient.currentScreen instanceof ShulkerBoxScreen) || (minecraftClient.currentScreen instanceof GenericContainerScreen)) {
            Iterator it = SearchHelper_4.minecraftClient.player.currentScreenHandler.slots.iterator();
            while (it.hasNext()) {
                Slot slot = (Slot) it.next();
                if (slot.getStack().isOf(item1241)) {
                    return slot.id;
                }
            }
        }
        return FireworksHelper.get442(item1241, xCarry.isToggled());
    }

    public Item getItem1241(boolean z) {
        return this.item;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }
}
