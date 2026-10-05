package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.v2.WrapWithCondition;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.render.Ambience;
import org.slf4j.Logger;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;

/* compiled from: 0.java */
@Mixin(targets = {"net.minecraft.client.option.GameOptions$3"})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinGameOptionsVisitor.class */
public class MixinGameOptionsVisitor {
    private static final Ambience ambience = (Ambience) BaritoneHelper_3.baritoneHelper_4.getModule117(Ambience.class);

    @WrapWithCondition(method = {"method_42572"}, at = {@At(value = "INVOKE", target = "Lorg/slf4j/Logger;error(Ljava/lang/String;)V")})
    private static boolean acceptHook(Logger logger, String str) {
        return ambience.brightness.getValue() != Ambience.MixinEntityRendererMode.GAMMA;
    }
}
