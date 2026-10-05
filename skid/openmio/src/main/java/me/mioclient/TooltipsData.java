package me.mioclient;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import me.mioclient.module.render.Tooltips;
import net.minecraft.component.DataComponentTypes;
import net.minecraft.component.type.ContainerComponent;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/TooltipsData.class */
public final class TooltipsData {
    public final ItemStack itemStack;
    public final ItemStack itemStack2;
    public static final Tooltips tooltips = (Tooltips) BaritoneHelper_3.baritoneHelper_4.getModule117(Tooltips.class);

    public TooltipsData(ItemStack itemStack, ItemStack itemStack2) {
        this.itemStack = itemStack;
        this.itemStack2 = itemStack2;
    }

    public static TooltipsData getTooltipsData2241(ItemStack itemStack) {
        ContainerComponent containerComponent;
        if (!AutoCrystalHelper.is135(itemStack.getItem()) || (containerComponent = (ContainerComponent) itemStack.get(DataComponentTypes.CONTAINER)) == null) {
            return null;
        }
        HashMap hashMap = new HashMap();
        List list = containerComponent.stream().toList();
        for (int i = 0; i < list.size(); i++) {
            Item item = ((ItemStack) list.get(i)).getItem();
            hashMap.put(item, Integer.valueOf(1 + ((Integer) hashMap.getOrDefault(item, 0)).intValue()));
        }
        if (hashMap.isEmpty()) {
            return null;
        }
        Map.Entry entry = (Map.Entry) hashMap.entrySet().stream().max(Map.Entry.comparingByValue()).orElse(null);
        if (entry == null) {
            return null;
        }
        return new TooltipsData(itemStack, ((Item) entry.getKey()).getDefaultStack());
    }




    public ItemStack getItemStack2242() {
        return this.itemStack;
    }

    public ItemStack getItemStack2243() {
        return this.itemStack2;
    }
}
