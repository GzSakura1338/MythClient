package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.EntityEvent_2;
import me.mioclient.SearchHelper_4;
import me.mioclient.ShaderSearchHelper4;
import me.mioclient.module.render.Chams;
import me.mioclient.module.render.NoRender;
import net.minecraft.client.network.ClientPlayerEntity;
import net.minecraft.client.render.VertexConsumer;
import net.minecraft.client.render.VertexConsumerProvider;
import net.minecraft.client.render.entity.EntityRenderDispatcher;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.entity.Entity;
import net.minecraft.entity.decoration.EndCrystalEntity;
import net.minecraft.world.WorldView;
import org.joml.Quaternionf;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({EntityRenderDispatcher.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinEntityRenderDispatcher.class */
public class MixinEntityRenderDispatcher {
    private static NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);
    private static Chams chams = (Chams) BaritoneHelper_3.baritoneHelper_4.getModule117(Chams.class);

    @Inject(method = {"renderShadow"}, at = {@At("HEAD")}, cancellable = true)
    private static void onRenderShadow(MatrixStack matrixStack, VertexConsumerProvider vertexConsumerProvider, Entity entity, float f, float f2, WorldView worldView, float f3, CallbackInfo callbackInfo) {
        if (chams.is2046(entity)) {
            callbackInfo.cancel();
        }
        if (ShaderSearchHelper4.flag) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"render"}, at = {@At("HEAD")}, cancellable = true)
    public <E extends Entity> void onRenderPre(E e, double d, double d2, double d3, float f, float f2, MatrixStack matrixStack, VertexConsumerProvider vertexConsumerProvider, int i, CallbackInfo callbackInfo) {
        EntityEvent_2.Inner inner994 = EntityEvent_2.Inner.getInner994(e, matrixStack, vertexConsumerProvider);
        SearchHelper_4.baritoneHelper.getObject1794(inner994);
        if (inner994.is2403() || e == null) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"render"}, at = {@At("RETURN")})
    public <E extends Entity> void onRenderPost(E e, double d, double d2, double d3, float f, float f2, MatrixStack matrixStack, VertexConsumerProvider vertexConsumerProvider, int i, CallbackInfo callbackInfo) {
        SearchHelper_4.baritoneHelper.getObject1794(EntityEvent_2.Inner_2.getInner_21412(e, matrixStack, vertexConsumerProvider));
    }

    @Inject(method = {"renderFire"}, at = {@At("HEAD")}, cancellable = true)
    private void renderFireHook(MatrixStack matrixStack, VertexConsumerProvider vertexConsumerProvider, Entity entity, Quaternionf quaternionf, CallbackInfo callbackInfo) {
        if (chams.is2046(entity)) {
            callbackInfo.cancel();
        }
        if (norender.isToggled() && norender.fire.getValue().booleanValue()) {
            if (norender.others.getValue().booleanValue() || (entity instanceof ClientPlayerEntity) || (entity instanceof EndCrystalEntity)) {
                callbackInfo.cancel();
            }
        }
    }

    @Inject(method = {"renderHitbox"}, at = {@At("HEAD")}, cancellable = true)
    private static void renderHitboxHook(MatrixStack matrixStack, VertexConsumer vertexConsumer, Entity entity, float f, float f2, float f3, float f4, CallbackInfo callbackInfo) {
        if (Chams.flag) {
            callbackInfo.cancel();
        }
    }
}
