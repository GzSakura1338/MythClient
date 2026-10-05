package me.mioclient.feature;

import it.unimi.dsi.fastutil.objects.Object2IntMap;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import me.mioclient.SearchHelper_4;
import net.minecraft.enchantment.Enchantment;
import net.minecraft.entity.EquipmentSlot;
import net.minecraft.entity.LivingEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.registry.RegistryKey;
import net.minecraft.registry.RegistryKeys;
import net.minecraft.registry.entry.RegistryEntry;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/feature/IllegalConstructorCall.class */
public final class IllegalConstructorCall implements SearchHelper_4 {
    public IllegalConstructorCall() {
        throw new IllegalArgumentException("Illegal constructor call");
    }

    public static int get1413(RegistryKey<Enchantment> registryKey, ItemStack itemStack) {
        if (itemStack.isEmpty()) {
            return 0;
        }
        for (Object2IntMap.Entry entry : itemStack.getEnchantments().getEnchantmentEntries()) {
            if (((RegistryEntry) entry.getKey()).matchesKey(registryKey)) {
                return entry.getIntValue();
            }
        }
        return 0;
    }

    public static int get1414(RegistryKey<Enchantment> registryKey, EquipmentSlot equipmentSlot, LivingEntity livingEntity) {
        return get1413(registryKey, livingEntity.getEquippedStack(equipmentSlot));
    }

    public static int get1415(RegistryKey<Enchantment> registryKey, EquipmentSlot equipmentSlot) {
        return get1414(registryKey, equipmentSlot, minecraftClient.player);
    }

    public static boolean is1416(RegistryKey<Enchantment> registryKey, ItemStack itemStack) {
        return get1413(registryKey, itemStack) > 0;
    }

    public static boolean is1417(RegistryKey<Enchantment> registryKey, EquipmentSlot equipmentSlot, LivingEntity livingEntity) {
        return get1414(registryKey, equipmentSlot, livingEntity) > 0;
    }

    public static boolean is1418(RegistryKey<Enchantment> registryKey, EquipmentSlot equipmentSlot) {
        return get1415(registryKey, equipmentSlot) > 0;
    }

    public static Map<RegistryKey<Enchantment>, Integer> getMap1419(ItemStack itemStack) {
        if (itemStack.isEmpty()) {
            return Collections.emptyMap();
        }
        HashMap hashMap = new HashMap();
        for (Object2IntMap.Entry entry : itemStack.getEnchantments().getEnchantmentEntries()) {
            Optional key = ((RegistryEntry) entry.getKey()).getKey();
            if (!key.isEmpty()) {
                hashMap.put((RegistryKey) key.get(), Integer.valueOf(entry.getIntValue()));
            }
        }
        return hashMap;
    }

    public static RegistryEntry<Enchantment> getRegistryEntry1420(RegistryKey<Enchantment> registryKey) {
        return (RegistryEntry) minecraftClient.world.getRegistryManager().get(RegistryKeys.ENCHANTMENT).getEntry(registryKey).orElse(null);
    }
}
