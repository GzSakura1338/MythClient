package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import com.llamalad7.mixinextras.injector.v2.WrapWithCondition;
import java.util.Collections;
import java.util.List;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.EntityEvent;
import me.mioclient.MixinMessageIndicatorHelper_2;
import me.mioclient.SearchHelper4_8;
import me.mioclient.SearchHelper_4;
import me.mioclient.ShaderSearchHelper4;
import me.mioclient.module.render.Animations;
import me.mioclient.module.render.Chams;
import me.mioclient.module.render.NoRender;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.render.VertexConsumer;
import net.minecraft.client.render.VertexConsumerProvider;
import net.minecraft.client.render.entity.LivingEntityRenderer;
import net.minecraft.client.render.entity.feature.FeatureRenderer;
import net.minecraft.client.render.entity.model.EntityModel;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.mob.WardenEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.ColorHelper;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin(value = {LivingEntityRenderer.class}, priority = 9999)
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinLivingEntityRenderer.class */
public abstract class MixinLivingEntityRenderer<T extends LivingEntity, M extends EntityModel<T>> implements SearchHelper_4 {
    private static final Chams chams = (Chams) BaritoneHelper_3.baritoneHelper_4.getModule117(Chams.class);
    private static final Animations animations = (Animations) BaritoneHelper_3.baritoneHelper_4.getModule117(Animations.class);
    private static final NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);

    @Unique
    private T lastEntity;

    @WrapWithCondition(method = {"render(Lnet/minecraft/entity/LivingEntity;FFLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;I)V"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/render/entity/model/EntityModel;render(Lnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumer;III)V")}, require = 0)
    private boolean onRenderModel(EntityModel entityModel, MatrixStack matrixStack, VertexConsumer vertexConsumer, int i, int i2, int i3) {
        if (ShaderSearchHelper4.flag) {
            return true;
        }
        EntityEvent entityEvent180 = EntityEvent.getEntityEvent180(this.lastEntity, i, ColorHelper.Argb.getRed(i3) / 255.0f, ColorHelper.Argb.getGreen(i3) / 255.0f, ColorHelper.Argb.getBlue(i3) / 255.0f, ColorHelper.Argb.getAlpha(i3) / 255.0f);
        baritoneHelper.getObject1794(entityEvent180);
        int i4 = MixinMessageIndicatorHelper_2.get822(entityEvent180.get183(), entityEvent180.get185(), entityEvent180.get187(), entityEvent180.get189());
        if (Chams.flag) {
            entityModel.render(matrixStack, chams.getTrajectoriesVertexConsumer2049().getBufferBuilder2596(), entityEvent180.get191(), i2);
            return false;
        }
        if (entityEvent180.is2403()) {
            return false;
        }
        entityModel.render(matrixStack, vertexConsumer, entityEvent180.get191(), i2, i4);
        return false;
    }

    @ModifyExpressionValue(method = {"render(Lnet/minecraft/entity/LivingEntity;FFLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;I)V"}, at = {@At(value = "FIELD", target = "Lnet/minecraft/client/render/entity/LivingEntityRenderer;features:Ljava/util/List;")})
    private List<FeatureRenderer<T, M>> render2(List<FeatureRenderer<T, M>> list) {
        return Chams.flag ? Collections.emptyList() : list;
    }

    @Redirect(method = {"render(Lnet/minecraft/entity/LivingEntity;FFLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;I)V"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/entity/LivingEntity;hasVehicle()Z", ordinal = 2))
    public boolean renderHook(LivingEntity livingEntity) {
        if (animations.is1000() && (livingEntity instanceof PlayerEntity)) {
            return true;
        }
        return livingEntity.hasVehicle();
    }

    @Redirect(method = {"render(Lnet/minecraft/entity/LivingEntity;FFLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;I)V"}, at = @At(value = "FIELD", target = "Lnet/minecraft/entity/LivingEntity;prevBodyYaw:F"))
    private float hook1(LivingEntity livingEntity) {
        return (livingEntity != minecraftClient.player || SearchHelper4_8.flag) ? livingEntity.prevBodyYaw : rotations().get2500();
    }

    @Redirect(method = {"render(Lnet/minecraft/entity/LivingEntity;FFLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;I)V"}, at = @At(value = "FIELD", target = "Lnet/minecraft/entity/LivingEntity;prevHeadYaw:F"))
    private float hook2(LivingEntity livingEntity) {
        return (livingEntity != minecraftClient.player || SearchHelper4_8.flag) ? livingEntity.prevHeadYaw : rotations().get2501();
    }

    @Redirect(method = {"render(Lnet/minecraft/entity/LivingEntity;FFLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;I)V"}, at = @At(value = "FIELD", target = "Lnet/minecraft/entity/LivingEntity;bodyYaw:F"))
    private float hook3(LivingEntity livingEntity) {
        return (livingEntity != minecraftClient.player || SearchHelper4_8.flag) ? livingEntity.bodyYaw : rotations().get2497();
    }

    @Redirect(method = {"render(Lnet/minecraft/entity/LivingEntity;FFLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;I)V"}, at = @At(value = "FIELD", target = "Lnet/minecraft/entity/LivingEntity;headYaw:F"))
    private float hook4(LivingEntity livingEntity) {
        return (livingEntity != minecraftClient.player || SearchHelper4_8.flag) ? livingEntity.headYaw : rotations().get2502();
    }

    @Redirect(method = {"render(Lnet/minecraft/entity/LivingEntity;FFLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;I)V"}, at = @At(value = "FIELD", target = "Lnet/minecraft/entity/LivingEntity;prevPitch:F"))
    private float hook5(LivingEntity livingEntity) {
        return (livingEntity != minecraftClient.player || SearchHelper4_8.flag) ? livingEntity.prevPitch : rotations().get2499();
    }

    @Redirect(method = {"render(Lnet/minecraft/entity/LivingEntity;FFLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;I)V"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/entity/LivingEntity;getPitch()F"))
    private float hook6(LivingEntity livingEntity) {
        return (livingEntity != minecraftClient.player || SearchHelper4_8.flag) ? livingEntity.getPitch() : rotations().get2496();
    }

    @Inject(method = {"render(Lnet/minecraft/entity/LivingEntity;FFLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;I)V"}, at = {@At("HEAD")})
    private void hook7(T t, float f, float f2, MatrixStack matrixStack, VertexConsumerProvider vertexConsumerProvider, int i, CallbackInfo callbackInfo) {
        this.lastEntity = t;
    }

    @Inject(method = {"render(Lnet/minecraft/entity/LivingEntity;FFLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;I)V"}, at = {@At("HEAD")}, cancellable = true)
    private void render(T t, float f, float f2, MatrixStack matrixStack, VertexConsumerProvider vertexConsumerProvider, int i, CallbackInfo callbackInfo) {
        int intValue = norender.wardenDistance.getValue().intValue();
        MinecraftClient minecraftClient = MinecraftClient.getInstance();
        if ((t instanceof WardenEntity) && norender.isToggled() && norender.wardens.getValue().booleanValue() && t.getBlockPos().getSquaredDistance(minecraftClient.gameRenderer.getCamera().getBlockPos()) > intValue * intValue) {
            callbackInfo.cancel();
        }
    }

    @ModifyExpressionValue(method = {"getOverlay"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/render/OverlayTexture;getV(Z)I")})
    private static int getV(int i) {
        if (norender.isToggled() && norender.hurt.getValue().booleanValue()) {
            return 10;
        }
        return i;
    }

    private SearchHelper4_8 rotations() {
        return BaritoneHelper_3.searchHelper4_8;
    }
}
