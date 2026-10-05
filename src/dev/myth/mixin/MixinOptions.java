package dev.myth.mixin;

import net.minecraft.client.Options;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.Constant;
import org.spongepowered.asm.mixin.injection.ModifyConstant;

/** Openmio MixinGameOptions: extend the vanilla FOV slider to 150. */
@Mixin(Options.class)
public abstract class MixinOptions {
    @ModifyConstant(method = "<init>", constant = @Constant(intValue = 110), require = 1, allow = 1)
    private int myth$fovLimit(int original) {
        return 150;
    }
}
