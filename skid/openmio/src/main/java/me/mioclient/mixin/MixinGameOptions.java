package me.mioclient.mixin;

import net.minecraft.client.option.GameOptions;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.Constant;
import org.spongepowered.asm.mixin.injection.ModifyConstant;

/* compiled from: 0.java */
@Mixin({GameOptions.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinGameOptions.class */
public class MixinGameOptions {
    @ModifyConstant(method = {"<init>"}, constant = {@Constant(intValue = 110)})
    private int bullet(int i) {
        return 150;
    }
}
