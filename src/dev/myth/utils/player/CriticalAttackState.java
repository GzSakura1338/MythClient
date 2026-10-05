package dev.myth.utils.player;

import net.minecraft.client.player.LocalPlayer;
import net.minecraft.world.effect.MobEffects;
import net.minecraft.world.level.block.WebBlock;
import net.minecraft.world.phys.AABB;

/** NovaClient GameCritical.cpp / game_critical.h attack gate. */
public record CriticalAttackState(double fallDistance, boolean water, boolean web, boolean ground,
                                  boolean climbing, boolean blind, boolean passenger, boolean sprinting) {
    public boolean allowsAttack() {
        return ground || water || web || (fallDistance > 0 && !climbing && !blind && !passenger && !sprinting);
    }

    /** Return null when the source's geometry or fall-distance checks fail. */
    public static CriticalAttackState read(LocalPlayer player) {
        if (player == null) return null;
        if (player.onGround()) return new CriticalAttackState(0, false, false, true, false, false, false, false);
        if (player.isInWater()) return new CriticalAttackState(0, true, false, false, false, false, false, false);
        AABB box = player.getBoundingBox();
        for (double edge : new double[]{box.minX, box.minY, box.minZ, box.maxX, box.maxY, box.maxZ})
            if (!Double.isFinite(edge) || Math.abs(edge) > 30000001) return null;
        if (box.getXsize() <= 0 || box.getYsize() <= 0 || box.getZsize() <= 0
                || box.getXsize() > 4 || box.getYsize() > 4 || box.getZsize() > 4) return null;
        // Cobwebs are non-solid: check every cell intersecting the body, including the head.
        var pos = new net.minecraft.core.BlockPos.MutableBlockPos();
        for (int x = (int) Math.floor(box.minX + .001); x <= (int) Math.floor(box.maxX - .001); x++)
            for (int y = (int) Math.floor(box.minY + .001); y <= (int) Math.floor(box.maxY - .001); y++)
                for (int z = (int) Math.floor(box.minZ + .001); z <= (int) Math.floor(box.maxZ - .001); z++)
                    if (player.level().getBlockState(pos.set(x, y, z)).getBlock() instanceof WebBlock)
                        return new CriticalAttackState(0, false, true, false, false, false, false, false);
        if (!Double.isFinite(player.fallDistance)) return null;
        return new CriticalAttackState(player.fallDistance, false, false, false, player.onClimbable(),
                player.hasEffect(MobEffects.BLINDNESS), player.isPassenger(), player.isSprinting());
    }
}
