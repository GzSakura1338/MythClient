package me.mioclient;

import net.minecraft.item.Item;
import net.minecraft.item.Items;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AntiPhaseMode.class */
public enum AntiPhaseMode implements EnumSettingHelper {
    LADDER("Ladder", Items.LADDER),
    VINE("Vine", Items.VINE),
    SCAFFOLDING("Scaffolding", Items.SCAFFOLDING),
    ITEMFRAME("ItemFrame", Items.ITEM_FRAME);

    public final String name;
    public final Item item;

    AntiPhaseMode(String str, Item item) {
        this.name = str;
        this.item = item;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }

    public Item getItem581() {
        return this.item;
    }
}
