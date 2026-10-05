package me.mioclient;

import me.mioclient.module.combat.AutoCrystal;
import net.minecraft.block.Blocks;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Items;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AutoCrystalMode_3.class */
public enum AutoCrystalMode_3 implements SearchHelper_4, EnumSettingHelper {
    PLACE("Place"),
    HIT("Hit");

    public final String name;
    public static AutoCrystal autoCrystal = (AutoCrystal) BaritoneHelper_3.baritoneHelper_4.getModule117(AutoCrystal.class);

    AutoCrystalMode_3(String str) {
        this.name = str;
    }

    public double get847(PlayerEntity playerEntity) {
        if (autoCrystal.flag3) {
            return 0.1d;
        }
        if (autoCrystal.facePlace.getValue().booleanValue() && (SearchHelper_3.get644((Entity) playerEntity) <= autoCrystal.health2.getValue().floatValue() || FireworksHelper.get451(playerEntity) <= autoCrystal.armorThreshold.getValue().intValue())) {
            return 0.6d;
        }
        if (minecraftClient.world.getBlockState(minecraftClient.player.getBlockPos()).isOf(Blocks.NETHER_PORTAL)) {
            return 0.6d;
        }
        double floatValue = autoCrystal.minDamage2.getValue().floatValue();
        double floatValue2 = autoCrystal.minDamage.getValue().floatValue();
        if (autoCrystal.minDamage2.is2348()) {
            floatValue = SearchHelper_3.get644((Entity) playerEntity);
        }
        if (autoCrystal.is1157()) {
            floatValue = Math.min(floatValue, 5.0d);
            floatValue2 = 3.0d;
        }
        return Math.min(this == HIT ? floatValue2 : floatValue, SearchHelper_3.get644((Entity) playerEntity));
    }

    public double get848() {
        if (autoCrystal.is1156()) {
            return 999.0d;
        }
        return (this == HIT ? autoCrystal.maxSelfDamage2.getValue() : autoCrystal.maxSelfDamage.getValue()).floatValue();
    }

    /* JADX WARN: Removed duplicated region for block: B:10:0x0081  */
    /* JADX WARN: Removed duplicated region for block: B:8:0x007f A[RETURN] */
    /*
        Code decompiled incorrectly, please refer to instructions dump.
    */
    public boolean is849(PlayerEntity playerEntity, double d, double d2, AutoCrystalMode_7 autoCrystalMode_7) {
        boolean z2 = minecraftClient.player.getOffHandStack().isOf(Items.TOTEM_OF_UNDYING)
                || minecraftClient.player.getMainHandStack().isOf(Items.TOTEM_OF_UNDYING);
        boolean equals = autoCrystalMode_7.equals(AutoCrystalMode_7.KILL);
        if (autoCrystal.forceSuicide.getValue().booleanValue()) {
            return false;
        }
        if (autoCrystal.is1156()) {
            d = 0.0d;
        }
        if (d >= SearchHelper_3.get643() && autoCrystal.antiSuicide.getValue().booleanValue() && (!equals || !z2)) {
            return true;
        }
        if (equals) {
            return false;
        }
        if (this != PLACE || d <= 0.0d || d2 / d >= autoCrystal.damageRatio.getValue().floatValue()) {
            return d2 < ((double) SearchHelper_3.get644((Entity) playerEntity)) && (d2 < get847(playerEntity) || d > get848());
        }
        return true;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }
}
