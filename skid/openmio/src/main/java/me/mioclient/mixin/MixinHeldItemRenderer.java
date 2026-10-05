package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.MixinHeldItemRendererEvent;
import me.mioclient.SearchHelper_4;
import me.mioclient.ShaderSearchHelper4;
import me.mioclient.module.misc.Swing;
import me.mioclient.module.render.Shader;
import me.mioclient.module.render.ViewModel;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.network.AbstractClientPlayerEntity;
import net.minecraft.client.render.OutlineVertexConsumerProvider;
import net.minecraft.client.render.VertexConsumerProvider;
import net.minecraft.client.render.item.HeldItemRenderer;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.component.DataComponentTypes;
import net.minecraft.item.ItemStack;
import net.minecraft.item.PotionItem;
import net.minecraft.util.Arm;
import net.minecraft.util.Hand;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.RotationAxis;
import org.joml.Quaternionf;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.ModifyArgs;
import org.spongepowered.asm.mixin.injection.ModifyVariable;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.invoke.arg.Args;

/* compiled from: 0.java */
@Mixin(value = {HeldItemRenderer.class}, priority = 9999)
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinHeldItemRenderer.class */
public abstract class MixinHeldItemRenderer {
    private static Shader shader = (Shader) BaritoneHelper_3.baritoneHelper_4.getModule117(Shader.class);
    private static ViewModel viewmodel = (ViewModel) BaritoneHelper_3.baritoneHelper_4.getModule117(ViewModel.class);
    private static Swing swing = (Swing) BaritoneHelper_3.baritoneHelper_4.getModule117(Swing.class);

    @Shadow
    private ItemStack field_4047;

    @Shadow
    private float field_4043;

    @Shadow
    @Final
    private MinecraftClient field_4050;

    @Shadow
    protected abstract void method_3228(AbstractClientPlayerEntity abstractClientPlayerEntity, float f, float f2, Hand hand, float f3, ItemStack itemStack, float f4, MatrixStack matrixStack, VertexConsumerProvider vertexConsumerProvider, int i);

    @Inject(method = {"renderFirstPersonItem"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/render/item/HeldItemRenderer;renderItem(Lnet/minecraft/entity/LivingEntity;Lnet/minecraft/item/ItemStack;Lnet/minecraft/client/render/model/json/ModelTransformationMode;ZLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;I)V")})
    private void onRenderItem(AbstractClientPlayerEntity abstractClientPlayerEntity, float f, float f2, Hand hand, float f3, ItemStack itemStack, float f4, MatrixStack matrixStack, VertexConsumerProvider vertexConsumerProvider, int i, CallbackInfo callbackInfo) {
        if (viewmodel.isToggled()) {
            boolean z = itemStack.contains(DataComponentTypes.FOOD) || (itemStack.getItem() instanceof PotionItem);
            boolean z2 = SearchHelper_4.minecraftClient.player.getActiveItem().getItem() == itemStack.getItem();
            if (z && SearchHelper_4.minecraftClient.player.getActiveItem() == itemStack && !viewmodel.eating.getValue().booleanValue()) {
                return;
            }
            if (hand == Hand.MAIN_HAND) {
                float f5 = (z && z2) ? 0.0f : -viewmodel.mainX.getValue().floatValue();
                float floatValue = (z && z2) ? 0.0f : viewmodel.mainZ.getValue().floatValue();
                if (SearchHelper_4.minecraftClient.player.getMainArm() == Arm.LEFT) {
                    f5 = -f5;
                }
                matrixStack.translate(f5, viewmodel.mainY.getValue().floatValue(), floatValue);
                matrixStack.scale(viewmodel.mainScaleX.getValue().floatValue(), viewmodel.mainScaleY.getValue().floatValue(), viewmodel.mainScaleZ.getValue().floatValue());
                matrixStack.multiply(RotationAxis.NEGATIVE_X.rotationDegrees(viewmodel.mainRotateX.getValue().floatValue()));
                matrixStack.multiply(RotationAxis.POSITIVE_Y.rotationDegrees(viewmodel.mainRotateY.getValue().floatValue()));
                matrixStack.multiply(RotationAxis.POSITIVE_Z.rotationDegrees(viewmodel.mainRotateZ.getValue().floatValue()));
                return;
            }
            float floatValue2 = (z && z2) ? 0.0f : viewmodel.offX.getValue().floatValue();
            float floatValue3 = (z && z2) ? 0.0f : viewmodel.offZ.getValue().floatValue();
            if (SearchHelper_4.minecraftClient.player.getMainArm() == Arm.LEFT) {
                floatValue2 = -floatValue2;
            }
            matrixStack.translate(floatValue2, viewmodel.offY.getValue().floatValue(), floatValue3);
            matrixStack.scale(viewmodel.offScaleX.getValue().floatValue(), viewmodel.offScaleY.getValue().floatValue(), viewmodel.offScaleZ.getValue().floatValue());
            matrixStack.multiply(RotationAxis.NEGATIVE_X.rotationDegrees(viewmodel.offRotateX.getValue().floatValue()));
            matrixStack.multiply(RotationAxis.NEGATIVE_Y.rotationDegrees(viewmodel.offRotateY.getValue().floatValue()));
            matrixStack.multiply(RotationAxis.NEGATIVE_Z.rotationDegrees(viewmodel.offRotateZ.getValue().floatValue()));
        }
    }

    @Inject(method = {"renderArmHoldingItem"}, at = {@At("HEAD")})
    private void renderArmHook(MatrixStack matrixStack, VertexConsumerProvider vertexConsumerProvider, int i, float f, float f2, Arm arm, CallbackInfo callbackInfo) {
        if (viewmodel.isToggled() && viewmodel.arm.getValue().booleanValue()) {
            float f3 = -viewmodel.mainX.getValue().floatValue();
            if (SearchHelper_4.minecraftClient.player.getMainArm() == Arm.LEFT) {
                f3 = -f3;
            }
            matrixStack.translate(f3, viewmodel.mainY.getValue().floatValue(), viewmodel.mainZ.getValue().floatValue());
            matrixStack.scale(viewmodel.mainScaleX.getValue().floatValue(), viewmodel.mainScaleY.getValue().floatValue(), viewmodel.mainScaleZ.getValue().floatValue());
        }
    }

    @Redirect(method = {"renderItem(FLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider$Immediate;Lnet/minecraft/client/network/ClientPlayerEntity;I)V"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/render/item/HeldItemRenderer;renderFirstPersonItem(Lnet/minecraft/client/network/AbstractClientPlayerEntity;FFLnet/minecraft/util/Hand;FLnet/minecraft/item/ItemStack;FLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;I)V"), require = 0)
    private void modifySwing(HeldItemRenderer heldItemRenderer, AbstractClientPlayerEntity abstractClientPlayerEntity, float f, float f2, Hand hand, float f3, ItemStack itemStack, float f4, MatrixStack matrixStack, VertexConsumerProvider vertexConsumerProvider, int i) {
        method_3228(abstractClientPlayerEntity, f, f2, hand, viewmodel.get3142(hand, f3), itemStack, f4, matrixStack, vertexConsumerProvider, i);
    }

    @ModifyVariable(method = {"renderFirstPersonItem"}, at = @At("HEAD"), argsOnly = true)
    private VertexConsumerProvider renderFirstPersonItemHook(VertexConsumerProvider vertexConsumerProvider) {
        if (!ShaderSearchHelper4.flag) {
            return vertexConsumerProvider;
        }
        OutlineVertexConsumerProvider outlineVertexConsumerProvider = shader.shader.getValue().getShaderFramebufferHelper21().outlineVertexConsumerProvider;
        outlineVertexConsumerProvider.setColor(255, 255, 255, 255);
        return outlineVertexConsumerProvider;
    }

    @Redirect(method = {"renderItem(FLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider$Immediate;Lnet/minecraft/client/network/ClientPlayerEntity;I)V"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/util/math/MatrixStack;multiply(Lorg/joml/Quaternionf;)V"))
    private void multiplyHook(MatrixStack matrixStack, Quaternionf quaternionf) {
        SearchHelper_4.baritoneHelper.getObject1794(new MixinHeldItemRendererEvent());
        if (viewmodel.isToggled() && viewmodel.noSway.getValue().booleanValue()) {
            return;
        }
        matrixStack.multiply(quaternionf);
    }

    @ModifyExpressionValue(method = {"updateHeldItems"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/util/math/MathHelper;clamp(FFF)F", ordinal = 2)})
    private float updateHeldItemsHook(float f) {
        if (!swing.isToggled() || swing.type.getValue() == Swing.MixinLivingEntityMode.VANILLA) {
            return f;
        }
        boolean z = swing.type.getValue() == Swing.MixinLivingEntityMode.ONE_EIGHT;
        float attackCooldownProgress = this.field_4050.player.getAttackCooldownProgress(1.0f);
        if (z) {
            attackCooldownProgress = 1.0f;
        }
        return MathHelper.clamp((!ItemStack.areItemsAndComponentsEqual(this.field_4047, SearchHelper_4.minecraftClient.player.getMainHandStack()) ? 0.0f : (attackCooldownProgress * attackCooldownProgress) * attackCooldownProgress) - this.field_4043, z ? -0.6f : -0.4f, 0.4f);
    }

    @ModifyExpressionValue(method = {"renderFirstPersonItem"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/network/AbstractClientPlayerEntity;isUsingRiptide()Z")})
    private boolean isUsingRiptideHook(boolean z) {
        if (viewmodel.isToggled() && viewmodel.noTridentAnim.getValue().booleanValue()) {
            return false;
        }
        return z;
    }

    @ModifyArgs(method = {"applyEquipOffset"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/util/math/MatrixStack;translate(FFF)V"))
    private void applyEquipOffsetHook(Args args) {
        if (viewmodel.isToggled() && viewmodel.instantSwap.getValue().booleanValue()) {
            args.set(1, Float.valueOf(-0.52f));
        }
    }

    @ModifyArgs(method = {"applyEatOrDrinkTransformation"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/util/math/MatrixStack;translate(FFF)V", ordinal = 0))
    private void applyEatOrDrinkTransformationHook(Args args) {
        if (viewmodel.isToggled()) {
            args.set(1, Float.valueOf(((Float) args.get(1)).floatValue() * viewmodel.eatMultiplier.getValue().floatValue()));
        }
    }
}
