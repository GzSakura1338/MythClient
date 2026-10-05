package me.mioclient;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import net.minecraft.client.util.math.MatrixStack;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/MatrixStackData.class */
public final class MatrixStackData {
    public final MatrixStack matrixStack;
    public final MatrixStackDataMode matrixStackDataMode;

    public MatrixStackData(MatrixStack matrixStack, MatrixStackDataMode matrixStackDataMode) {
        this.matrixStack = matrixStack;
        this.matrixStackDataMode = matrixStackDataMode;
    }




    public MatrixStack getMatrixStack1013() {
        return this.matrixStack;
    }

    public MatrixStackDataMode getMatrixStackDataMode1014() {
        return this.matrixStackDataMode;
    }
}
