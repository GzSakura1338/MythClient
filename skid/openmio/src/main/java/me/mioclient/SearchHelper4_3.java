package me.mioclient;

import java.util.Iterator;
import me.mioclient.mixin.ducks.DuckBoatEntityRenderer;
import net.minecraft.client.model.ModelPart;
import net.minecraft.client.render.entity.model.CompositeEntityModel;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.entity.vehicle.BoatEntity;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.RotationAxis;
import org.joml.Quaternionf;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper4_3.class */
public class SearchHelper4_3 implements SearchHelper_4, Helper_6<BoatEntity> {
    public static final Quaternionf quaternionf = new Quaternionf();

    @Override // me.mioclient.Helper_6
    /* renamed from: do1637, reason: merged with bridge method [inline-methods] */
    public void do721(BoatEntity boatEntity, float f, MatrixStack matrixStack) {
        DuckBoatEntityRenderer renderer = (DuckBoatEntityRenderer) minecraftClient.getEntityRenderDispatcher().getRenderer(boatEntity);
        matrixStack.push();
        matrixStack.translate(0.0f, 0.375f, 0.0f);
        matrixStack.multiply(RotationAxis.POSITIVE_Y.rotationDegrees(180.0f - MathHelper.lerp(f, boatEntity.prevYaw, boatEntity.getYaw())));
        float damageWobbleTicks = boatEntity.getDamageWobbleTicks() - f;
        float damageWobbleStrength = boatEntity.getDamageWobbleStrength() - f;
        if (damageWobbleStrength < 0.0f) {
            damageWobbleStrength = 0.0f;
        }
        if (damageWobbleTicks > 0.0f) {
            matrixStack.multiply(RotationAxis.POSITIVE_X.rotationDegrees((((MathHelper.sin(damageWobbleTicks) * damageWobbleTicks) * damageWobbleStrength) / 10.0f) * boatEntity.getDamageWobbleSide()));
        }
        if (!MathHelper.approximatelyEquals(boatEntity.interpolateBubbleWobble(f), 0.0f)) {
            matrixStack.multiply(quaternionf.setAngleAxis(boatEntity.interpolateBubbleWobble(f) * FreecamHelper.val4, 1.0f, 0.0f, 1.0f));
        }
        CompositeEntityModel compositeEntityModel = (CompositeEntityModel) renderer.mio$getTexturesAndModels().get(boatEntity.getVariant()).getSecond();
        matrixStack.scale(-1.0f, -1.0f, 1.0f);
        matrixStack.multiply(RotationAxis.POSITIVE_Y.rotationDegrees(FreecamHelper.num2));
        compositeEntityModel.setAngles(boatEntity, f, 0.0f, -0.1f, 0.0f, 0.0f);
        Iterator it = compositeEntityModel.getParts().iterator();
        while (it.hasNext()) {
            ChamsHelper_2.do617(new MatrixStackData(matrixStack, MatrixStackDataMode.BOTH), (ModelPart) it.next());
        }
        matrixStack.pop();
    }
}
