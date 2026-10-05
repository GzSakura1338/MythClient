package me.mioclient;

import com.google.common.collect.ImmutableList;
import net.minecraft.client.model.ModelPart;
import net.minecraft.client.render.entity.model.EntityModel;
import net.minecraft.client.render.entity.model.RabbitEntityModel;
import net.minecraft.client.util.math.MatrixStack;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper42_2.class */
public class SearchHelper42_2 extends SearchHelper4_2 {
    @Override // me.mioclient.SearchHelper4_2
    public void do1540(MatrixStackData matrixStackData, EntityModel<?> entityModel) {
        MatrixStack matrixStack1013 = matrixStackData.getMatrixStack1013();
        if (entityModel instanceof RabbitEntityModel) {
            RabbitEntityModel rabbitEntityModel = (RabbitEntityModel) entityModel;
            if (!rabbitEntityModel.child) {
                matrixStack1013.push();
                matrixStack1013.scale(0.6f, 0.6f, 0.6f);
                matrixStack1013.translate(0.0f, 1.0f, 0.0f);
                ImmutableList.of(rabbitEntityModel.leftHindLeg, rabbitEntityModel.rightHindLeg, rabbitEntityModel.leftHaunch, rabbitEntityModel.rightHaunch, rabbitEntityModel.body, rabbitEntityModel.leftFrontLeg, rabbitEntityModel.rightFrontLeg, rabbitEntityModel.head, rabbitEntityModel.rightEar, rabbitEntityModel.leftEar, rabbitEntityModel.tail, rabbitEntityModel.nose, new ModelPart[0]).forEach(modelPart -> {
                    ChamsHelper_2.do617(matrixStackData, modelPart);
                });
                matrixStack1013.pop();
                return;
            }
            matrixStack1013.push();
            matrixStack1013.scale(0.56666666f, 0.56666666f, 0.56666666f);
            matrixStack1013.translate(0.0f, 1.375f, 0.125f);
            ImmutableList.of(rabbitEntityModel.head, rabbitEntityModel.leftEar, rabbitEntityModel.rightEar, rabbitEntityModel.nose).forEach(modelPart2 -> {
                ChamsHelper_2.do617(matrixStackData, modelPart2);
            });
            matrixStack1013.pop();
            matrixStack1013.push();
            matrixStack1013.scale(0.4f, 0.4f, 0.4f);
            matrixStack1013.translate(0.0f, 2.25f, 0.0f);
            ImmutableList.of(rabbitEntityModel.leftHindLeg, rabbitEntityModel.rightHindLeg, rabbitEntityModel.leftHaunch, rabbitEntityModel.rightHaunch, rabbitEntityModel.body, rabbitEntityModel.leftFrontLeg, rabbitEntityModel.rightFrontLeg, rabbitEntityModel.tail).forEach(modelPart3 -> {
                ChamsHelper_2.do617(matrixStackData, modelPart3);
            });
            matrixStack1013.pop();
        }
    }
}
