package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import com.llamalad7.mixinextras.sugar.Local;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.FreecamHelper;
import me.mioclient.Helper_18;
import me.mioclient.ShaderSearchHelper4;
import me.mioclient.module.movement.ElytraFly;
import me.mioclient.module.render.Animations;
import me.mioclient.module.render.NoRender;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.network.AbstractClientPlayerEntity;
import net.minecraft.client.render.RenderLayer;
import net.minecraft.client.render.entity.EntityRendererFactory;
import net.minecraft.client.render.entity.LivingEntityRenderer;
import net.minecraft.client.render.entity.PlayerEntityRenderer;
import net.minecraft.client.render.entity.model.PlayerEntityModel;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.util.Identifier;
import net.minecraft.util.math.RotationAxis;
import net.minecraft.util.math.Vec3d;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.ModifyArgs;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;
import org.spongepowered.asm.mixin.injection.invoke.arg.Args;

/* compiled from: 0.java */
@Mixin({PlayerEntityRenderer.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinPlayerEntityRenderer.class */
public abstract class MixinPlayerEntityRenderer extends LivingEntityRenderer<AbstractClientPlayerEntity, PlayerEntityModel<AbstractClientPlayerEntity>> {
    private static Animations animations = (Animations) BaritoneHelper_3.baritoneHelper_4.getModule117(Animations.class);
    private static final NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);
    private static final ElytraFly elytrafly = (ElytraFly) BaritoneHelper_3.baritoneHelper_4.getModule117(ElytraFly.class);

    @Unique
    AbstractClientPlayerEntity rendered;

    public MixinPlayerEntityRenderer(EntityRendererFactory.Context context, PlayerEntityModel<AbstractClientPlayerEntity> playerEntityModel, float f) {
        super(context, playerEntityModel, f);
    }

    @Inject(method = {"getPositionOffset(Lnet/minecraft/client/network/AbstractClientPlayerEntity;F)Lnet/minecraft/util/math/Vec3d;"}, at = {@At("HEAD")}, cancellable = true)
    private void mio$getPositionOffset(AbstractClientPlayerEntity abstractClientPlayerEntity, float f, CallbackInfoReturnable<Vec3d> callbackInfoReturnable) {
        if (!animations.is1001() || abstractClientPlayerEntity == MinecraftClient.getInstance().player) {
            return;
        }
        callbackInfoReturnable.setReturnValue(new Vec3d(0.0d, (abstractClientPlayerEntity.getScale() * (-2.0f)) / 16.0d, 0.0d));
    }

    @ModifyExpressionValue(method = {"setModelPose(Lnet/minecraft/client/network/AbstractClientPlayerEntity;)V"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/network/AbstractClientPlayerEntity;isInSneakingPose()Z")})
    private boolean mio$isInSneakingPose(boolean z, @Local(argsOnly = true) AbstractClientPlayerEntity abstractClientPlayerEntity) {
        if (!animations.is1001() || abstractClientPlayerEntity == MinecraftClient.getInstance().player) {
            return z;
        }
        return true;
    }

    @Inject(method = {"scale(Lnet/minecraft/client/network/AbstractClientPlayerEntity;Lnet/minecraft/client/util/math/MatrixStack;F)V"}, at = {@At("HEAD")})
    private void scalePre(AbstractClientPlayerEntity abstractClientPlayerEntity, MatrixStack matrixStack, float f, CallbackInfo callbackInfo) {
        this.rendered = abstractClientPlayerEntity;
    }

    @ModifyArgs(method = {"scale(Lnet/minecraft/client/network/AbstractClientPlayerEntity;Lnet/minecraft/client/util/math/MatrixStack;F)V"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/util/math/MatrixStack;scale(FFF)V"))
    private void scaleHook(Args args) {
        if (animations.isToggled() && animations.is999() && this.rendered != MinecraftClient.getInstance().player) {
            args.set(0, Float.valueOf(((Float) args.get(0)).floatValue() * animations.playerScale.getValue().floatValue()));
            args.set(1, Float.valueOf(((Float) args.get(1)).floatValue() * animations.playerScale.getValue().floatValue()));
            args.set(2, Float.valueOf(((Float) args.get(2)).floatValue() * animations.playerScale.getValue().floatValue()));
        }
    }

    @ModifyArgs(method = {"setupTransforms(Lnet/minecraft/client/network/AbstractClientPlayerEntity;Lnet/minecraft/client/util/math/MatrixStack;FFFF)V"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/util/math/MatrixStack;multiply(Lorg/joml/Quaternionf;)V"))
    private void setupTransformsHook(Args args) {
        if (elytrafly.isToggled() && elytrafly.mode.getValue() == ElytraFly.ElytraFlyPredicateMode.BOUNCE && elytrafly.is951() && elytrafly.pitchLock.getValue().booleanValue() && elytrafly.silent.getValue().booleanValue()) {
            args.set(0, RotationAxis.POSITIVE_X.rotationDegrees(-FreecamHelper.num));
        }
    }

    @ModifyExpressionValue(method = {"renderArm"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/render/RenderLayer;getEntityTranslucent(Lnet/minecraft/util/Identifier;)Lnet/minecraft/client/render/RenderLayer;")})
    private RenderLayer renderArmHook(RenderLayer renderLayer, @Local Identifier identifier) {
        return (norender.isToggled() && norender.hands.getValue().booleanValue() && ShaderSearchHelper4.flag) ? RenderLayer.getEntitySolid(identifier) : renderLayer;
    }

    @ModifyExpressionValue(method = {"renderArm"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/render/RenderLayer;getEntitySolid(Lnet/minecraft/util/Identifier;)Lnet/minecraft/client/render/RenderLayer;")})
    private RenderLayer renderArm(RenderLayer renderLayer, @Local Identifier identifier) {
        return norender.get1995() != 1.0f ? Helper_18.getRenderLayer2032(identifier) : renderLayer;
    }

    protected /* synthetic */ float getShadowRadius(AbstractClientPlayerEntity p0) {
        return super.getShadowRadius(p0);
    }
}
