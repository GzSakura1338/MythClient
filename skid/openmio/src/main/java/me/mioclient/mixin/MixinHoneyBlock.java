package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.movement.NoSlow;
import net.minecraft.block.HoneyBlock;
import net.minecraft.entity.Entity;
import net.minecraft.util.math.BlockPos;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Redirect;

/* compiled from: 0.java */
@Mixin({HoneyBlock.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinHoneyBlock.class */
public class MixinHoneyBlock {
    private static NoSlow noslow = (NoSlow) BaritoneHelper_3.baritoneHelper_4.getModule117(NoSlow.class);

    @Shadow
    private boolean method_23356(BlockPos blockPos, Entity entity) {
        return false;
    }

    @Redirect(method = {"onEntityCollision"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/block/HoneyBlock;isSliding(Lnet/minecraft/util/math/BlockPos;Lnet/minecraft/entity/Entity;)Z"))
    private boolean onEntityCollisionHook(HoneyBlock honeyBlock, BlockPos blockPos, Entity entity) {
        if (noslow.isToggled() && noslow.honey.getValue().booleanValue()) {
            return false;
        }
        return method_23356(blockPos, entity);
    }
}
