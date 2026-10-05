package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import com.llamalad7.mixinextras.injector.WrapWithCondition;
import com.llamalad7.mixinextras.sugar.Local;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.ChamsMode;
import me.mioclient.Helper_18;
import me.mioclient.ShaderSearchHelper4;
import me.mioclient.SpawnTimeHelper_2;
import me.mioclient.module.combat.AutoCrystal;
import me.mioclient.module.render.Animations;
import me.mioclient.module.render.Chams;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.model.ModelPart;
import net.minecraft.client.render.VertexConsumer;
import net.minecraft.client.render.VertexConsumerProvider;
import net.minecraft.client.render.entity.EndCrystalEntityRenderer;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.entity.decoration.EndCrystalEntity;
import net.minecraft.util.Identifier;
import net.minecraft.util.math.MathHelper;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.ModifyArgs;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;
import org.spongepowered.asm.mixin.injection.invoke.arg.Args;

/* compiled from: 0.java */
@Mixin({EndCrystalEntityRenderer.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinEndCrystalEntityRenderer.class */
public class MixinEndCrystalEntityRenderer {

    @Shadow
    @Final
    private static Identifier field_4663;
    private static AutoCrystal ac = (AutoCrystal) BaritoneHelper_3.baritoneHelper_4.getModule117(AutoCrystal.class);
    private static Chams mod = (Chams) BaritoneHelper_3.baritoneHelper_4.getModule117(Chams.class);
    private static Animations animations = (Animations) BaritoneHelper_3.baritoneHelper_4.getModule117(Animations.class);

    @Unique
    private static EndCrystalEntity last;

    @Unique
    private int mio$ordinal;

    @ModifyArgs(method = {"render*"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/util/math/MatrixStack;scale(FFF)V", ordinal = 0))
    private void scaleHook(Args args) {
        if (animations.isToggled() && animations.crystals.getValue().booleanValue()) {
            args.set(0, Float.valueOf(((Float) args.get(0)).floatValue() * animations.crystalScale.getValue().floatValue()));
            args.set(1, Float.valueOf(((Float) args.get(1)).floatValue() * animations.crystalScale.getValue().floatValue()));
            args.set(2, Float.valueOf(((Float) args.get(2)).floatValue() * animations.crystalScale.getValue().floatValue()));
        }
    }

    @Inject(method = {"render(Lnet/minecraft/entity/decoration/EndCrystalEntity;FFLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;I)V"}, at = {@At("HEAD")}, cancellable = true)
    private void renderHook(EndCrystalEntity endCrystalEntity, float f, float f2, MatrixStack matrixStack, VertexConsumerProvider vertexConsumerProvider, int i, CallbackInfo callbackInfo) {
        this.mio$ordinal = 0;
        last = endCrystalEntity;
        if (System.currentTimeMillis() - ((SpawnTimeHelper_2) endCrystalEntity).getSpawnTime() > Math.max(BaritoneHelper_3.holeSnapSearchHelper4_4.get1730(), 50) && endCrystalEntity.age < 10 && ac.isToggled() && ac.inhibit.getValue().booleanValue() && ((SpawnTimeHelper_2) endCrystalEntity).isMioAttacked()) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"getYOffset"}, at = {@At("HEAD")}, cancellable = true)
    private static void offsetHook(EndCrystalEntity endCrystalEntity, float f, CallbackInfoReturnable<Float> callbackInfoReturnable) {
        if (animations.isToggled() && animations.crystals.getValue().booleanValue()) {
            float sin = (MathHelper.sin((endCrystalEntity.endCrystalAge + f) * 0.2f) / 2.0f) + 0.5f;
            float f2 = 1.0f;
            if (animations.isToggled() && animations.crystals.getValue().booleanValue()) {
                f2 = animations.floatFactor.getValue().floatValue();
            }
            if (endCrystalEntity.age < 10 && ac.isToggled() && animations.floatFactor.getValue().floatValue() != 0.0f && ac.inhibit.getValue().booleanValue() && ((SpawnTimeHelper_2) endCrystalEntity).isMioAttacked()) {
                f2 *= ((SpawnTimeHelper_2) endCrystalEntity).getSpawnTime() % 2 == 0 ? 0.0f : 1.0f;
            }
            callbackInfoReturnable.cancel();
            callbackInfoReturnable.setReturnValue(Float.valueOf((((sin * sin) + sin) * f2) - 1.4f));
        }
    }

    @ModifyArgs(method = {"render(Lnet/minecraft/entity/decoration/EndCrystalEntity;FFLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;I)V"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/util/math/RotationAxis;rotationDegrees(F)Lorg/joml/Quaternionf;"), require = 0)
    private void speedHook(Args args) {
        if (last.age < 10 && ac.isToggled() && ac.inhibit.getValue().booleanValue() && ((me.mioclient.SpawnTimeHelper_2)(Object) last).isMioAttacked()) {
            args.set(0, Float.valueOf(((Float) args.get(0)).floatValue() * 1.5f));
        }
        if (animations.isToggled() && animations.crystals.getValue().booleanValue()) {
            args.set(0, Float.valueOf(((Float) args.get(0)).floatValue() * animations.rotationSpeed.getValue().floatValue()));
        }
    }

    @WrapWithCondition(method = {"render(Lnet/minecraft/entity/decoration/EndCrystalEntity;FFLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;I)V"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/model/ModelPart;render(Lnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumer;II)V")})
    private boolean renderHook(ModelPart modelPart, MatrixStack matrixStack, VertexConsumer vertexConsumer, int i, int i2) {
        boolean mio$renderHandler = mio$renderHandler(modelPart, matrixStack, vertexConsumer, i, i2, this.mio$ordinal);
        this.mio$ordinal++;
        return mio$renderHandler;
    }

    @Inject(method = {"render(Lnet/minecraft/entity/decoration/EndCrystalEntity;FFLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;I)V"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/util/math/MatrixStack;multiply(Lorg/joml/Quaternionf;)V", ordinal = 0)})
    private void renderHook2(EndCrystalEntity endCrystalEntity, float f, float f2, MatrixStack matrixStack, VertexConsumerProvider vertexConsumerProvider, int i, CallbackInfo callbackInfo) {
        if (this.mio$ordinal == 0) {
            this.mio$ordinal++;
        }
    }

    @ModifyExpressionValue(method = {"render(Lnet/minecraft/entity/decoration/EndCrystalEntity;FFLnet/minecraft/client/util/math/MatrixStack;Lnet/minecraft/client/render/VertexConsumerProvider;I)V"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/render/VertexConsumerProvider;getBuffer(Lnet/minecraft/client/render/RenderLayer;)Lnet/minecraft/client/render/VertexConsumer;")})
    private VertexConsumer render3(VertexConsumer vertexConsumer, @Local(argsOnly = true) VertexConsumerProvider vertexConsumerProvider) {
        return (!mod.isToggled() || !mod.xqz.getValue().booleanValue() || mod.crystals.getValue() == ChamsMode.OFF || ShaderSearchHelper4.flag) ? vertexConsumer : vertexConsumerProvider.getBuffer(Helper_18.getRenderLayer2033(field_4663));
    }

    @Unique
    private boolean mio$renderHandler(ModelPart modelPart, MatrixStack matrixStack, VertexConsumer vertexConsumer, int i, int i2, int i3) {
        if (animations.is1003(i3)) {
            return false;
        }
        if (!mod.isToggled() || !mod.crystals.getValue().is594() || MinecraftClient.getInstance().gameRenderer.getCamera().getPos().distanceTo(last.getPos()) > mod.range.getValue().intValue() || !mod.is1763(last)) {
            return true;
        }
        render(modelPart, matrixStack, i2);
        return !mod.is2046(last);
    }

    private static void render(ModelPart modelPart, MatrixStack matrixStack, int i) {
        if (mod.isToggled() && mod.crystals.getValue().is594() && mod.shine2.getValue().booleanValue() && Chams.flag) {
            matrixStack.push();
            modelPart.render(matrixStack, mod.getTrajectoriesVertexConsumer2049().getBufferBuilder2596(), 15728880, i);
            matrixStack.pop();
        }
    }
}
