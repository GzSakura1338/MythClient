package me.mioclient;

import me.mioclient.module.combat.AntiPhase;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Direction;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AntiPhaseHelper3.class */
public class AntiPhaseHelper3 implements AntiPhaseHelper_3 {
    @Override // me.mioclient.AntiPhaseHelper_3
    public AntiPhaseData getAntiPhaseData844(AntiPhase antiPhase, PlayerEntity playerEntity) {
        BlockPos blockPos2008 = HoleSnapSearchHelper4.getBlockPos2008((LivingEntity) playerEntity);
        Direction direction3029 = PhaseESPSearchHelper4_2.getDirection3029(blockPos2008, antiPhase.strictDirection.getValue().booleanValue());
        if (direction3029 == null || !SearchHelper4_7.is2446(blockPos2008)) {
            return null;
        }
        return new AntiPhaseData(blockPos2008.offset(direction3029), direction3029.getOpposite(), getBox2417(blockPos2008, direction3029));
    }

    public Box getBox2417(BlockPos blockPos, Direction direction) {
        return new Box(blockPos);
    }
}
