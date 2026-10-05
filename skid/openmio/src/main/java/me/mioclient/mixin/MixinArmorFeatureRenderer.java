package me.mioclient.mixin;

import java.awt.Color;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.Helper_18;
import me.mioclient.MixinMessageIndicatorHelper_2;
import me.mioclient.module.render.Chams;
import me.mioclient.module.render.NoRender;
import net.minecraft.client.render.RenderLayer;
import net.minecraft.client.render.VertexConsumerProvider;
import net.minecraft.client.render.entity.feature.ArmorFeatureRenderer;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.entity.LivingEntity;
import net.minecraft.util.Identifier;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.ModifyArgs;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.invoke.arg.Args;

/* compiled from: 0.java */
@Mixin({ArmorFeatureRenderer.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinArmorFeatureRenderer.class */
public class MixinArmorFeatureRenderer {
    private static final NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);
    private static final Chams chams = (Chams) BaritoneHelper_3.baritoneHelper_4.getModule117(Chams.class);

    @Unique
    private LivingEntity mio$lastEntity;

    @Inject(method = {"render(Lnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;ILnet/minecraft/entity/LivingEntity;FFFFFF)V"}, at = {@At("HEAD")}, cancellable = true)
    private <T extends LivingEntity> void renderHook(MatrixStack matrixStack, VertexConsumerProvider vertexConsumerProvider, int i, T t, float f, float f2, float f3, float f4, float f5, float f6, CallbackInfo callbackInfo) {
        this.mio$lastEntity = t;
        if (norender.get1996() == 0.0f) {
            callbackInfo.cancel();
        }
    }

    @ModifyArgs(method = {"renderArmorParts"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/render/entity/model/BipedEntityModel;render(Lnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumer;III)V"))
    private void renderArmorParts(Args args) {
        args.set(4, Integer.valueOf(MixinMessageIndicatorHelper_2.getColor817(new Color(((Integer) args.get(4)).intValue()), norender.get1996() * norender.get1997()).hashCode()));
    }

    @Redirect(method = {"renderArmorParts"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/render/RenderLayer;getArmorCutoutNoCull(Lnet/minecraft/util/Identifier;)Lnet/minecraft/client/render/RenderLayer;"))
    private RenderLayer renerArmorParts(Identifier identifier) {
        float f = norender.get1996() * norender.get1997();
        if (chams.is2045(this.mio$lastEntity)) {
            f *= chams.opacity.getValue().intValue();
        }
        return f == 1.0f ? RenderLayer.getArmorCutoutNoCull(identifier) : Helper_18.getRenderLayer2031(identifier);
    }
}
