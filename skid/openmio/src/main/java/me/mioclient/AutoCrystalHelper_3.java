package me.mioclient;

import net.minecraft.component.DataComponentTypes;
import net.minecraft.component.type.PotionContentsComponent;
import net.minecraft.entity.effect.StatusEffectInstance;
import net.minecraft.item.ItemStack;
import net.minecraft.potion.Potion;
import net.minecraft.registry.entry.RegistryEntry;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AutoCrystalHelper_3.class */
public class AutoCrystalHelper_3 {
    public static final Potion potion = new Potion(new StatusEffectInstance[0]);

    public static Potion getPotion1565(ItemStack itemStack) {
        RegistryEntry registryEntry;
        PotionContentsComponent potionContentsComponent = (PotionContentsComponent) itemStack.get(DataComponentTypes.POTION_CONTENTS);
        if (potionContentsComponent != null && (registryEntry = (RegistryEntry) potionContentsComponent.potion().orElse(null)) != null) {
            return (Potion) registryEntry.value();
        }
        return potion;
    }
}
