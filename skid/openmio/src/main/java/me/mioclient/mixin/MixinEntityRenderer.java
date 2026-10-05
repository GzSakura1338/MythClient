package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.SearchHelper_4;
import me.mioclient.ShaderSearchHelper4;
import me.mioclient.event.RenderLabelEvent;
import me.mioclient.module.render.Ambience;
import me.mioclient.module.render.NameTags;
import net.minecraft.client.render.VertexConsumerProvider;
import net.minecraft.client.render.entity.EntityRenderer;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.text.Text;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({EntityRenderer.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinEntityRenderer.class */
public abstract class MixinEntityRenderer<T extends Entity> {
    private static NameTags nametags = (NameTags) BaritoneHelper_3.baritoneHelper_4.getModule117(NameTags.class);
    private static Ambience ambience = (Ambience) BaritoneHelper_3.baritoneHelper_4.getModule117(Ambience.class);

    @Inject(method = {"renderLabelIfPresent"}, at = {@At("HEAD")}, cancellable = true)
    private void onRenderLabel(T t, Text text, MatrixStack matrixStack, VertexConsumerProvider vertexConsumerProvider, int i, float f, CallbackInfo callbackInfo) {
        if (ShaderSearchHelper4.flag) {
            callbackInfo.cancel();
            return;
        }
        RenderLabelEvent renderLabelEvent = new RenderLabelEvent(matrixStack, t);
        SearchHelper_4.baritoneHelper.getObject1794(renderLabelEvent);
        if (renderLabelEvent.is2403()) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"hasLabel"}, at = {@At("HEAD")}, cancellable = true)
    private void hasLabelHook(T t, CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        if ((t instanceof PlayerEntity) && nametags.isToggled()) {
            callbackInfoReturnable.cancel();
            callbackInfoReturnable.setReturnValue(Boolean.valueOf(t.hasCustomName()));
        }
    }

    @Inject(method = {"getSkyLight"}, at = {@At("RETURN")}, cancellable = true)
    private void getSkyLightHook(CallbackInfoReturnable<Integer> callbackInfoReturnable) {
        if (ambience.isToggled() && ambience.brightness.getValue() == Ambience.MixinEntityRendererMode.SKY) {
            callbackInfoReturnable.setReturnValue(Integer.valueOf(Math.max(ambience.lightLevel.getValue().intValue(), callbackInfoReturnable.getReturnValueI())));
        }
    }
}
