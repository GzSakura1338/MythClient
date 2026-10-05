package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.movement.NoSlow;
import net.minecraft.block.BlockState;
import net.minecraft.block.SlimeBlock;
import net.minecraft.entity.Entity;
import net.minecraft.util.math.BlockPos;
import net.minecraft.world.World;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({SlimeBlock.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinSlimeBlock.class */
public class MixinSlimeBlock {
    private static NoSlow noslow = (NoSlow) BaritoneHelper_3.baritoneHelper_4.getModule117(NoSlow.class);

    @Inject(method = {"onSteppedOn"}, at = {@At("HEAD")}, cancellable = true)
    private void onSteppedOnHook(World world, BlockPos blockPos, BlockState blockState, Entity entity, CallbackInfo callbackInfo) {
        if (noslow.isToggled() && noslow.slime.getValue().booleanValue()) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"bounce"}, at = {@At("HEAD")}, cancellable = true)
    private void bounceHook(Entity entity, CallbackInfo callbackInfo) {
        if (noslow.isToggled() && noslow.slime.getValue().booleanValue() && noslow.noSlimeBounce.getValue().booleanValue()) {
            callbackInfo.cancel();
        }
    }
}
