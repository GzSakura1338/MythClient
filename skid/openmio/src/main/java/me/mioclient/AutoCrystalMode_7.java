package me.mioclient;

import me.mioclient.module.combat.AutoCrystal;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.item.Items;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AutoCrystalMode_7.class */
public enum AutoCrystalMode_7 implements SearchHelper_4 {
    NONE(false),
    POP(true),
    KILL(true);

    public static AutoCrystal autoCrystal = (AutoCrystal) BaritoneHelper_3.baritoneHelper_4.getModule117(AutoCrystal.class);
    public final boolean flag;

    AutoCrystalMode_7(boolean z) {
        this.flag = z;
    }

    /* JADX WARN: Removed duplicated region for block: B:11:0x0066  */
    /* JADX WARN: Removed duplicated region for block: B:18:0x0073  */
    /*
        Code decompiled incorrectly, please refer to instructions dump.
    */
    public static AutoCrystalMode_7 getAutoCrystalMode_71882(LivingEntity livingEntity, double d) {
        boolean z2 = d >= ((double) SearchHelper_3.get644((Entity) livingEntity));
        boolean z = livingEntity.getOffHandStack().isOf(Items.TOTEM_OF_UNDYING)
                || livingEntity.getMainHandStack().isOf(Items.TOTEM_OF_UNDYING);
        return !z2 ? (z ? POP : KILL) : NONE;
    }
}
