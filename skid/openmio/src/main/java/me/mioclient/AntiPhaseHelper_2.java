package me.mioclient;

import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.BlockPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AntiPhaseHelper_2.class */
public class AntiPhaseHelper_2 extends AntiPhaseHelper {
    @Override // me.mioclient.AntiPhaseHelper
    public boolean is845(BlockPos blockPos, PlayerEntity playerEntity, AntiPhaseHelperData antiPhaseHelperData) {
        return SearchHelper.getBox234((LivingEntity) playerEntity).intersects(antiPhaseHelperData.getBox799().offset(blockPos));
    }
}
