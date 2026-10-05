package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.render.ESP;
import me.mioclient.module.render.NoRender;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.render.VertexConsumerProvider;
import net.minecraft.client.render.block.entity.BlockEntityRenderDispatcher;
import net.minecraft.client.util.math.MatrixStack;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({BlockEntityRenderDispatcher.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinBlockEntityRenderDispatcher.class */
public class MixinBlockEntityRenderDispatcher {
    private static final ESP esp = (ESP) BaritoneHelper_3.baritoneHelper_4.getModule117(ESP.class);
    private static NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);

    @Inject(method = {"render(Lnet/minecraft/block/entity/BlockEntity;FLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;)V"}, at = {@At("HEAD")}, cancellable = true)
    private <E extends BlockEntity> void renderHook(E e, float f, MatrixStack matrixStack, VertexConsumerProvider vertexConsumerProvider, CallbackInfo callbackInfo) {
        if (!(norender.ignoreESP.getValue().booleanValue() && esp.is1918()) && norender.isToggled() && norender.tileEntities.getValue().booleanValue() && e.getPos().getSquaredDistance(MinecraftClient.getInstance().gameRenderer.getCamera().getBlockPos()) > norender.tileDistance.getValue().intValue() * norender.tileDistance.getValue().intValue()) {
            callbackInfo.cancel();
        }
    }
}
