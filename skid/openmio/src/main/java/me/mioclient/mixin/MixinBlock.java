package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.movement.NoSlow;
import me.mioclient.module.render.Xray;
import net.minecraft.block.Block;
import net.minecraft.block.BlockState;
import net.minecraft.block.Blocks;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Direction;
import net.minecraft.world.BlockView;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({Block.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinBlock.class */
public class MixinBlock {
    private static NoSlow noslow = (NoSlow) BaritoneHelper_3.baritoneHelper_4.getModule117(NoSlow.class);

    @Inject(method = {"shouldDrawSide"}, at = {@At("HEAD")}, cancellable = true)
    private static void onShouldDrawHook(BlockState blockState, BlockView blockView, BlockPos blockPos, Direction direction, BlockPos blockPos2, CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        if (Xray.getXray3073().isToggled()) {
            callbackInfoReturnable.cancel();
            callbackInfoReturnable.setReturnValue(Boolean.valueOf(Xray.getXray3073().is3071(blockPos, blockState.getBlock())));
        }
    }

    @Inject(method = {"getSlipperiness"}, at = {@At("HEAD")}, cancellable = true)
    private void getSlipperinessHook(CallbackInfoReturnable<Float> callbackInfoReturnable) {
        if (noslow.isToggled()) {
            Block block = (Block)(Object) this;
            if (noslow.ice.getValue().booleanValue() && (block == Blocks.ICE || block == Blocks.PACKED_ICE || block == Blocks.BLUE_ICE)) {
                callbackInfoReturnable.setReturnValue(Float.valueOf(0.6f));
            }
            if (noslow.slime.getValue().booleanValue() && block == Blocks.SLIME_BLOCK) {
                callbackInfoReturnable.setReturnValue(Float.valueOf(0.6f));
            }
            if (noslow.honey.getValue().booleanValue() && block == Blocks.HONEY_BLOCK) {
                callbackInfoReturnable.setReturnValue(Float.valueOf(0.6f));
            }
        }
    }
}
