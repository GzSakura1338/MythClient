package me.mioclient;

import net.minecraft.client.MinecraftClient;
import net.minecraft.client.model.ModelPart;
import net.minecraft.client.render.entity.EnderDragonEntityRenderer;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.entity.boss.dragon.EnderDragonEntity;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.RotationAxis;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper4_16.class */
public class SearchHelper4_16 implements SearchHelper_4, Helper_6<EnderDragonEntity> {
    @Override // me.mioclient.Helper_6
    /* renamed from: do718, reason: merged with bridge method [inline-methods] */
    public void do721(EnderDragonEntity enderDragonEntity, float f, MatrixStack matrixStack) {
        EnderDragonEntityRenderer renderer = (EnderDragonEntityRenderer) MinecraftClient.getInstance().getEntityRenderDispatcher().getRenderer(enderDragonEntity);
        matrixStack.push();
        float f2 = (float) enderDragonEntity.getSegmentProperties(7, f)[0];
        float f3 = (float) (enderDragonEntity.getSegmentProperties(5, f)[1] - enderDragonEntity.getSegmentProperties(10, f)[1]);
        matrixStack.multiply(RotationAxis.POSITIVE_Y.rotationDegrees(-f2));
        matrixStack.multiply(RotationAxis.POSITIVE_X.rotationDegrees(f3 * 10.0f));
        matrixStack.translate(0.0f, 0.0f, 1.0f);
        matrixStack.scale(-1.0f, -1.0f, 1.0f);
        matrixStack.translate(0.0f, -1.501f, 0.0f);
        renderer.model.animateModel(enderDragonEntity, 0.0f, 0.0f, f);
        do719(matrixStack, enderDragonEntity, renderer.model, f);
        matrixStack.pop();
    }

    public void do719(MatrixStack matrixStack, EnderDragonEntity enderDragonEntity, EnderDragonEntityRenderer.DragonEntityModel dragonEntityModel, float f) {
        MatrixStackData matrixStackData = new MatrixStackData(matrixStack, MatrixStackDataMode.BOTH);
        matrixStack.push();
        float lerp = MathHelper.lerp(f, enderDragonEntity.prevWingPosition, enderDragonEntity.wingPosition);
        dragonEntityModel.jaw.pitch = ((float) (Math.sin(lerp * 6.2831855f) + 1.0d)) * 0.2f;
        float sin = (float) (Math.sin((lerp * 6.2831855f) - 1.0f) + 1.0d);
        float f2 = ((sin * sin) + (sin * 2.0f)) * 0.05f;
        matrixStack.translate(0.0f, f2 - 2.0f, -3.0f);
        matrixStack.multiply(RotationAxis.POSITIVE_X.rotationDegrees(f2 * 2.0f));
        float f3 = 0.0f;
        float f4 = 20.0f;
        float f5 = -12.0f;
        double[] segmentProperties = enderDragonEntity.getSegmentProperties(6, f);
        float wrapDegrees = MathHelper.wrapDegrees((float) (enderDragonEntity.getSegmentProperties(5, f)[0] - enderDragonEntity.getSegmentProperties(10, f)[0]));
        float wrapDegrees2 = MathHelper.wrapDegrees((float) (enderDragonEntity.getSegmentProperties(5, f)[0] + (wrapDegrees / 2.0f)));
        float f6 = lerp * 6.2831855f;
        for (int i = 0; i < 5; i++) {
            double[] segmentProperties2 = enderDragonEntity.getSegmentProperties(5 - i, f);
            float cos = ((float) Math.cos((i * 0.45f) + f6)) * 0.15f;
            dragonEntityModel.neck.yaw = MathHelper.wrapDegrees((float) (segmentProperties2[0] - segmentProperties[0])) * FreecamHelper.val4 * 1.5f;
            dragonEntityModel.neck.pitch = cos + (enderDragonEntity.getChangeInNeckPitch(i, segmentProperties, segmentProperties2) * FreecamHelper.val4 * 1.5f * 5.0f);
            dragonEntityModel.neck.roll = (-MathHelper.wrapDegrees((float) (segmentProperties2[0] - wrapDegrees2))) * FreecamHelper.val4 * 1.5f;
            dragonEntityModel.neck.pivotY = f4;
            dragonEntityModel.neck.pivotZ = f5;
            dragonEntityModel.neck.pivotX = f3;
            f4 += MathHelper.sin(dragonEntityModel.neck.pitch) * 10.0f;
            f5 -= (MathHelper.cos(dragonEntityModel.neck.yaw) * MathHelper.cos(dragonEntityModel.neck.pitch)) * 10.0f;
            f3 -= (MathHelper.sin(dragonEntityModel.neck.yaw) * MathHelper.cos(dragonEntityModel.neck.pitch)) * 10.0f;
            ChamsHelper_2.do617(matrixStackData, dragonEntityModel.neck);
        }
        dragonEntityModel.head.pivotY = f4;
        dragonEntityModel.head.pivotZ = f5;
        dragonEntityModel.head.pivotX = f3;
        double[] segmentProperties3 = enderDragonEntity.getSegmentProperties(0, f);
        dragonEntityModel.head.yaw = MathHelper.wrapDegrees((float) (segmentProperties3[0] - segmentProperties[0])) * FreecamHelper.val4;
        dragonEntityModel.head.pitch = MathHelper.wrapDegrees(enderDragonEntity.getChangeInNeckPitch(6, segmentProperties, segmentProperties3)) * FreecamHelper.val4 * 1.5f * 5.0f;
        dragonEntityModel.head.roll = (-MathHelper.wrapDegrees((float) (segmentProperties3[0] - wrapDegrees2))) * FreecamHelper.val4;
        ChamsHelper_2.do617(matrixStackData, dragonEntityModel.head);
        matrixStack.push();
        matrixStack.translate(0.0f, 1.0f, 0.0f);
        matrixStack.multiply(RotationAxis.POSITIVE_Z.rotationDegrees((-wrapDegrees) * 1.5f));
        matrixStack.translate(0.0f, -1.0f, 0.0f);
        dragonEntityModel.body.roll = 0.0f;
        ChamsHelper_2.do617(matrixStackData, dragonEntityModel.body);
        float f7 = lerp * 6.2831855f;
        dragonEntityModel.leftWing.pitch = 0.125f - (((float) Math.cos(f7)) * 0.2f);
        dragonEntityModel.leftWing.yaw = -0.25f;
        dragonEntityModel.leftWing.roll = (-((float) (Math.sin(f7) + 0.125d))) * 0.8f;
        dragonEntityModel.leftWingTip.roll = ((float) (Math.sin(f7 + 2.0f) + FreecamHelper.val2)) * 0.75f;
        dragonEntityModel.rightWing.pitch = dragonEntityModel.leftWing.pitch;
        dragonEntityModel.rightWing.yaw = -dragonEntityModel.leftWing.yaw;
        dragonEntityModel.rightWing.roll = -dragonEntityModel.leftWing.roll;
        dragonEntityModel.rightWingTip.roll = -dragonEntityModel.leftWingTip.roll;
        do720(matrixStackData, f2, dragonEntityModel.leftWing, dragonEntityModel.leftFrontLeg, dragonEntityModel.leftFrontLegTip, dragonEntityModel.leftFrontFoot, dragonEntityModel.leftHindLeg, dragonEntityModel.leftHindLegTip, dragonEntityModel.leftHindFoot);
        do720(matrixStackData, f2, dragonEntityModel.rightWing, dragonEntityModel.rightFrontLeg, dragonEntityModel.rightFrontLegTip, dragonEntityModel.rightFrontFoot, dragonEntityModel.rightHindLeg, dragonEntityModel.rightHindLegTip, dragonEntityModel.rightHindFoot);
        matrixStack.pop();
        float f8 = (-MathHelper.sin(lerp * 6.2831855f)) * 0.0f;
        float f9 = lerp * 6.2831855f;
        float f10 = 10.0f;
        float f11 = 60.0f;
        float f12 = 0.0f;
        double[] segmentProperties4 = enderDragonEntity.getSegmentProperties(11, f);
        for (int i2 = 0; i2 < 12; i2++) {
            double[] segmentProperties5 = enderDragonEntity.getSegmentProperties(12 + i2, f);
            f8 += MathHelper.sin((i2 * 0.45f) + f9) * 0.05f;
            dragonEntityModel.neck.yaw = ((MathHelper.wrapDegrees((float) (segmentProperties5[0] - segmentProperties4[0])) * 1.5f) + 180.0f) * FreecamHelper.val4;
            dragonEntityModel.neck.pitch = f8 + (((float) (segmentProperties5[1] - segmentProperties4[1])) * FreecamHelper.val4 * 1.5f * 5.0f);
            dragonEntityModel.neck.roll = MathHelper.wrapDegrees((float) (segmentProperties5[0] - wrapDegrees2)) * FreecamHelper.val4 * 1.5f;
            dragonEntityModel.neck.pivotY = f10;
            dragonEntityModel.neck.pivotZ = f11;
            dragonEntityModel.neck.pivotX = f12;
            f10 += MathHelper.sin(dragonEntityModel.neck.pitch) * 10.0f;
            f11 -= (MathHelper.cos(dragonEntityModel.neck.yaw) * MathHelper.cos(dragonEntityModel.neck.pitch)) * 10.0f;
            f12 -= (MathHelper.sin(dragonEntityModel.neck.yaw) * MathHelper.cos(dragonEntityModel.neck.pitch)) * 10.0f;
            ChamsHelper_2.do617(matrixStackData, dragonEntityModel.neck);
        }
        matrixStack.pop();
    }

    public void do720(MatrixStackData matrixStackData, float f, ModelPart modelPart, ModelPart modelPart2, ModelPart modelPart3, ModelPart modelPart4, ModelPart modelPart5, ModelPart modelPart6, ModelPart modelPart7) {
        modelPart5.pitch = 1.0f + (f * 0.1f);
        modelPart6.pitch = (float) (FreecamHelper.val2 + (f * 0.1f));
        modelPart7.pitch = 0.75f + (f * 0.1f);
        modelPart2.pitch = 1.3f + (f * 0.1f);
        modelPart3.pitch = (float) ((-FreecamHelper.val2) - (f * 0.1f));
        modelPart4.pitch = 0.75f + (f * 0.1f);
        ChamsHelper_2.do617(matrixStackData, modelPart);
        ChamsHelper_2.do617(matrixStackData, modelPart2);
        ChamsHelper_2.do617(matrixStackData, modelPart5);
    }
}
