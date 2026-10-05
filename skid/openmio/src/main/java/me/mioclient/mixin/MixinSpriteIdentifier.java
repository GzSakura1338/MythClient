package me.mioclient.mixin;

import com.mojang.blaze3d.systems.RenderSystem;
import java.util.function.Function;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.render.NoRender;
import net.minecraft.client.render.RenderLayer;
import net.minecraft.client.util.SpriteIdentifier;
import net.minecraft.util.Identifier;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({SpriteIdentifier.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinSpriteIdentifier.class */
public class MixinSpriteIdentifier {

    @Shadow
    @Final
    private Identifier field_21769;
    private static final NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);

    @Inject(method = {"getRenderLayer"}, at = {@At("HEAD")}, cancellable = true)
    private void getRenderLayerHook(Function<Identifier, RenderLayer> function, CallbackInfoReturnable<RenderLayer> callbackInfoReturnable) {
        if (norender.get1995() == 1.0f || RenderSystem.getShaderColor()[3] == 1.0f) {
            return;
        }
        callbackInfoReturnable.setReturnValue(RenderLayer.getEntityTranslucent(this.field_21769));
        callbackInfoReturnable.cancel();
    }
}
