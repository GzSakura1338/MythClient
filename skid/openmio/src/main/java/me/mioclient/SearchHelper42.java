package me.mioclient;

import com.google.common.collect.ImmutableList;
import net.minecraft.client.render.entity.model.EntityModel;
import net.minecraft.client.render.entity.model.LlamaEntityModel;
import net.minecraft.client.util.math.MatrixStack;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper42.class */
public class SearchHelper42 extends SearchHelper4_2 {
    @Override // me.mioclient.SearchHelper4_2
    public void do1540(MatrixStackData matrixStackData, EntityModel<?> entityModel) {
        MatrixStack matrixStack1013 = matrixStackData.getMatrixStack1013();
        if (entityModel instanceof LlamaEntityModel) {
            LlamaEntityModel llamaEntityModel = (LlamaEntityModel) entityModel;
            if (!llamaEntityModel.child) {
                ImmutableList.of(llamaEntityModel.head, llamaEntityModel.body, llamaEntityModel.rightHindLeg, llamaEntityModel.leftHindLeg, llamaEntityModel.rightFrontLeg, llamaEntityModel.leftFrontLeg, llamaEntityModel.rightChest, llamaEntityModel.leftChest).forEach(modelPart -> {
                    ChamsHelper_2.do617(matrixStackData, modelPart);
                });
                return;
            }
            matrixStack1013.push();
            matrixStack1013.scale(0.71428573f, 0.64935064f, 0.7936508f);
            matrixStack1013.translate(0.0f, 1.3125f, 0.22f);
            ChamsHelper_2.do617(matrixStackData, llamaEntityModel.head);
            matrixStack1013.pop();
            matrixStack1013.push();
            matrixStack1013.scale(0.625f, 0.45454544f, 0.45454544f);
            matrixStack1013.translate(0.0f, 2.0625f, 0.0f);
            ChamsHelper_2.do617(matrixStackData, llamaEntityModel.body);
            matrixStack1013.pop();
            matrixStack1013.push();
            matrixStack1013.scale(0.45454544f, 0.41322312f, 0.45454544f);
            matrixStack1013.translate(0.0f, 2.0625f, 0.0f);
            ImmutableList.of(llamaEntityModel.rightHindLeg, llamaEntityModel.leftHindLeg, llamaEntityModel.rightFrontLeg, llamaEntityModel.leftFrontLeg, llamaEntityModel.rightChest, llamaEntityModel.leftChest).forEach(modelPart2 -> {
                ChamsHelper_2.do617(matrixStackData, modelPart2);
            });
            matrixStack1013.pop();
        }
    }
}
