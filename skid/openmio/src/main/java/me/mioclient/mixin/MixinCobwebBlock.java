package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.movement.FastWeb;
import net.minecraft.block.BlockState;
import net.minecraft.block.CobwebBlock;
import net.minecraft.entity.Entity;
import net.minecraft.util.math.BlockPos;
import net.minecraft.world.World;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({CobwebBlock.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinCobwebBlock.class */
public class MixinCobwebBlock {
    private static final FastWeb fastweb = (FastWeb) BaritoneHelper_3.baritoneHelper_4.getModule117(FastWeb.class);

    @Inject(method = {"onEntityCollision"}, at = {@At("HEAD")}, cancellable = true)
    private void onEntityCollisionHook(BlockState blockState, World world, BlockPos blockPos, Entity entity, CallbackInfo callbackInfo) {
        if (fastweb.is1534()) {
            callbackInfo.cancel();
        }
    }
}
