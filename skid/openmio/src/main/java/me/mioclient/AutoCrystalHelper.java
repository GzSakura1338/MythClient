package me.mioclient;

import java.util.Iterator;
import net.minecraft.block.BedBlock;
import net.minecraft.block.ShulkerBoxBlock;
import net.minecraft.entity.effect.StatusEffect;
import net.minecraft.entity.effect.StatusEffectInstance;
import net.minecraft.item.BlockItem;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.registry.entry.RegistryEntry;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AutoCrystalHelper.class */
public final class AutoCrystalHelper {
    public AutoCrystalHelper() {
        throw new AssertionError();
    }

    public static boolean is135(Item item) {
        return (item instanceof BlockItem) && (((BlockItem) item).getBlock() instanceof ShulkerBoxBlock);
    }

    public static boolean is136(Item item) {
        return (item instanceof BlockItem) && (((BlockItem) item).getBlock() instanceof BedBlock);
    }

    public static boolean is137(ItemStack itemStack, RegistryEntry<StatusEffect> registryEntry) {
        Iterator it = AutoCrystalHelper_3.getPotion1565(itemStack).getEffects().iterator();
        while (it.hasNext()) {
            if (((StatusEffectInstance) it.next()).getEffectType() == registryEntry) {
                return true;
            }
        }
        return false;
    }
}
