package me.mioclient;

import net.minecraft.entity.LivingEntity;
import net.minecraft.item.ItemStack;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/NameTagsHelper.class */
public interface NameTagsHelper {
    long mio$getLastEatingTime();

    NameTagsHelperMode mio$getRole();

    void mio$setRole(NameTagsHelperMode nameTagsHelperMode);

    boolean mio$isNextToWall();

    default float get70(ItemStack itemStack, LivingEntity livingEntity) {
        return ((float) (System.currentTimeMillis() - mio$getLastEatingTime())) / ((itemStack.getMaxUseTime(livingEntity) * 50.0f) + 50.0f);
    }
}
