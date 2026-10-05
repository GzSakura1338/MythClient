package me.mioclient;

import me.mioclient.module.combat.BedAura;
import net.minecraft.block.BedBlock;
import net.minecraft.block.entity.BedBlockEntity;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.block.enums.BedPart;
import net.minecraft.entity.LivingEntity;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/BedAuraSearchHelper46.class */
public class BedAuraSearchHelper46 implements SearchHelper4_6<LivingEntity, BlockPos> {
    public final BedAura bedAura;
    public Vec3d vec3d;
    public float val;
    public float val2;
    public float val3;

    public BedAuraSearchHelper46(BedAura bedAura) {
        this.bedAura = bedAura;
    }

    public void do466() {
        this.vec3d = minecraftClient.player.getEyePos();
        this.val = this.bedAura.range.getValue().floatValue();
        this.val2 = this.bedAura.minDamage.getValue().floatValue();
        this.val3 = this.bedAura.maxSelfDamage.getValue().floatValue();
    }

    @Override // me.mioclient.SearchHelper4_6
    /* renamed from: getBlockPos3124, reason: merged with bridge method [inline-methods] */
    public BlockPos getObject970(LivingEntity livingEntity) {
        do466();
        float f = 0.0f;
        BlockPos blockPos = null;
        for (BlockEntity blockEntity : BaritoneHelper_3.stashFinderSearchHelper4.getList1555()) {
            if (blockEntity instanceof BedBlockEntity) {
                if (blockEntity.getCachedState().get(BedBlock.PART) != BedPart.FOOT) {
                    if (blockEntity.getPos().toCenterPos().distanceTo(this.vec3d) <= this.val) {
                        float f2 = ArmorSearchHelper4.get1897(blockEntity.getPos().toCenterPos(), livingEntity);
                        float f3 = ArmorSearchHelper4.get1897(blockEntity.getPos().toCenterPos(), minecraftClient.player);
                        if (f3 < SearchHelper_3.get643() || !this.bedAura.antiSucide.getValue().booleanValue()) {
                            if (f3 <= this.val3 && f2 >= this.val2 && (blockPos == null || f2 > f)) {
                                blockPos = blockEntity.getPos();
                                f = f2;
                            }
                        }
                    }
                }
            }
        }
        return blockPos;
    }
}
