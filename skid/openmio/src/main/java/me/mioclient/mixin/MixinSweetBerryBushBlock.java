package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.movement.NoSlow;
import net.minecraft.block.BlockState;
import net.minecraft.block.SweetBerryBushBlock;
import net.minecraft.entity.Entity;
import net.minecraft.util.math.Vec3d;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Redirect;

/* compiled from: 0.java */
@Mixin({SweetBerryBushBlock.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinSweetBerryBushBlock.class */
public class MixinSweetBerryBushBlock {
    private static NoSlow noslow = (NoSlow) BaritoneHelper_3.baritoneHelper_4.getModule117(NoSlow.class);

    @Redirect(method = {"onEntityCollision"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/entity/Entity;slowMovement(Lnet/minecraft/block/BlockState;Lnet/minecraft/util/math/Vec3d;)V"))
    private void onEntityCollisionHook(Entity entity, BlockState blockState, Vec3d vec3d) {
        if (noslow.isToggled() && noslow.berryBush.getValue().booleanValue()) {
            return;
        }
        entity.slowMovement(blockState, vec3d);
    }
}
