package me.mioclient;

import me.mioclient.mixin.ducks.DuckEndCrystalEntityRenderer;
import me.mioclient.module.combat.AutoCrystal;
import me.mioclient.module.render.Animations;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.render.entity.EndCrystalEntityRenderer;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.entity.decoration.EndCrystalEntity;
import net.minecraft.util.math.RotationAxis;
import org.joml.Quaternionf;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/QuaternionfHelper.class */
public class QuaternionfHelper implements Helper_6<EndCrystalEntity> {
    public static AutoCrystal ac = (AutoCrystal) BaritoneHelper_3.baritoneHelper_4.getModule117(AutoCrystal.class);
    public static Animations animations = (Animations) BaritoneHelper_3.baritoneHelper_4.getModule117(Animations.class);
    public static final float val = (float) Math.sin(0.7853981633974483d);
    public final Quaternionf quaternionf = new Quaternionf();
    public EndCrystalEntity last;

    @Override // me.mioclient.Helper_6
    /* renamed from: do1025, reason: merged with bridge method [inline-methods] */
    public void do721(EndCrystalEntity endCrystalEntity, float f, MatrixStack matrixStack) {
        this.last = endCrystalEntity;
        MatrixStackData matrixStackData = new MatrixStackData(matrixStack, MatrixStackDataMode.BOTH);
        DuckEndCrystalEntityRenderer renderer = (DuckEndCrystalEntityRenderer) MinecraftClient.getInstance().getEntityRenderDispatcher().getRenderer(endCrystalEntity);
        matrixStack.push();
        float yOffset = EndCrystalEntityRenderer.getYOffset(endCrystalEntity, f);
        float f2 = (endCrystalEntity.endCrystalAge + f) * 3.0f;
        matrixStack.scale(2.0f * get989(), 2.0f * get989(), 2.0f * get989());
        matrixStack.translate(0.0d, -FreecamHelper.val2, 0.0d);
        if (endCrystalEntity.shouldShowBottom() && !animations.is1007()) {
            ChamsHelper_2.do617(matrixStackData, renderer.mio$getBottom());
        }
        matrixStack.multiply(RotationAxis.POSITIVE_Y.rotationDegrees(f2 * get1026()));
        matrixStack.translate(0.0f, 1.5f + (yOffset / 2.0f), 0.0f);
        matrixStack.multiply(this.quaternionf.setAngleAxis(1.0471976f, val, 0.0f, val));
        if (!animations.is1005()) {
            ChamsHelper_2.do617(matrixStackData, renderer.mio$getFrame());
        }
        matrixStack.scale(0.875f, 0.875f, 0.875f);
        matrixStack.multiply(this.quaternionf.setAngleAxis(1.0471976f, val, 0.0f, val));
        matrixStack.multiply(RotationAxis.POSITIVE_Y.rotationDegrees(f2 * get1026()));
        if (!animations.is1004()) {
            ChamsHelper_2.do617(matrixStackData, renderer.mio$getFrame());
        }
        matrixStack.pop();
    }

    public float get989() {
        if (animations.isToggled() && animations.crystals.getValue().booleanValue()) {
            return animations.crystalScale.getValue().floatValue();
        }
        return 1.0f;
    }

    public float get1026() {
        if (this.last.age < 10 && ac.isToggled() && ac.inhibit.getValue().booleanValue() && ((SpawnTimeHelper_2)(Object) this.last).isMioAttacked()) {
            return 1.5f;
        }
        if (animations.isToggled() && animations.crystals.getValue().booleanValue()) {
            return animations.rotationSpeed.getValue().floatValue();
        }
        return 1.0f;
    }
}
