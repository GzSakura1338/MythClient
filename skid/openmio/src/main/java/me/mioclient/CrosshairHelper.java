package me.mioclient;

import com.mojang.blaze3d.systems.RenderSystem;
import java.awt.Color;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.render.GameRenderer;
import net.minecraft.client.render.VertexFormat;
import net.minecraft.client.render.VertexFormats;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.text.OrderedText;
import org.joml.Matrix4f;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/CrosshairHelper.class */
public class CrosshairHelper {
    public static final TrajectoriesVertexConsumer trajectoriesVertexConsumer = TrajectoriesVertexConsumer.getTrajectoriesVertexConsumer2592();

    public static void do1703(MatrixStack matrixStack, float f, float f2, float f3, float f4, Color color) {
        do1707(matrixStack, f, f3, f2 + f4, f3 + f4, color);
    }

    public static void do1704(MatrixStack matrixStack, float f, float f2, float f3, float f4, Color color) {
        do1707(matrixStack, f, f2 + f4, f + f4, f3, color);
    }

    public static void do1705(MatrixStack matrixStack, float f, float f2, float f3, float f4, Color color) {
        do1703(matrixStack, f, f3, f2, 1.0f, color);
        do1704(matrixStack, f3, f2, f4, 1.0f, color);
        do1703(matrixStack, f, f3, f4, 1.0f, color);
        do1704(matrixStack, f, f2, f4, 1.0f, color);
    }

    public static void do1706(MatrixStack matrixStack, float f, float f2, float f3, float f4, float f5, Color color) {
        do1703(matrixStack, f, f3, f2, f5, color);
        do1704(matrixStack, f3, f2, f4, f5, color);
        do1703(matrixStack, f, f3, f4, f5, color);
        do1704(matrixStack, f, f2, f4, f5, color);
    }

    public static void do1707(MatrixStack matrixStack, float f, float f2, float f3, float f4, Color color) {
        do1708(matrixStack, f, f2, f3, f4, color.hashCode());
    }

    public static void do1708(MatrixStack matrixStack, float f, float f2, float f3, float f4, int i) {
        float f5 = ((i >> 24) & 255) / 255.0f;
        float f6 = ((i >> 16) & 255) / 255.0f;
        float f7 = ((i >> 8) & 255) / 255.0f;
        float f8 = (i & 255) / 255.0f;
        if (f5 == 0.0f) {
            return;
        }
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), f, f4, 0.0f).color(f6, f7, f8, f5);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), f3, f4, 0.0f).color(f6, f7, f8, f5);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), f3, f2, 0.0f).color(f6, f7, f8, f5);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), f, f2, 0.0f).color(f6, f7, f8, f5);
    }

    public static void do1709(Matrix4f matrix4f, float f, float f2, float f3, float f4, int i, int i2, int i3, int i4) {
        float[] fArr = {((i >> 16) & 255) / 255.0f, ((i >> 8) & 255) / 255.0f, (i & 255) / 255.0f};
        float[] fArr2 = {((i2 >> 16) & 255) / 255.0f, ((i2 >> 8) & 255) / 255.0f, (i2 & 255) / 255.0f};
        float[] fArr3 = {((i3 >> 16) & 255) / 255.0f, ((i3 >> 8) & 255) / 255.0f, (i3 & 255) / 255.0f};
        float[] fArr4 = {((i4 >> 16) & 255) / 255.0f, ((i4 >> 8) & 255) / 255.0f, (i4 & 255) / 255.0f};
        trajectoriesVertexConsumer.vertex(matrix4f, f, f2 + f4, 0.0f).color(fArr4[0], fArr4[1], fArr4[2], 1.0f);
        trajectoriesVertexConsumer.vertex(matrix4f, f + f3, f2 + f4, 0.0f).color(fArr3[0], fArr3[1], fArr3[2], 1.0f);
        trajectoriesVertexConsumer.vertex(matrix4f, f + f3, f2, 0.0f).color(fArr2[0], fArr2[1], fArr2[2], 1.0f);
        trajectoriesVertexConsumer.vertex(matrix4f, f, f2, 0.0f).color(fArr[0], fArr[1], fArr[2], 1.0f);
    }

    public static void do1710(Matrix4f matrix4f, float f, float f2, float f3, float f4, int... iArr) {
        if (iArr.length < 2) {
            return;
        }
        float[][] fArr = new float[iArr.length][4];
        for (int i = 0; i < iArr.length; i++) {
            float[] fArr2 = new float[4];
            fArr2[0] = ((iArr[i] >> 24) & 255) / 255.0f;
            fArr2[1] = ((iArr[i] >> 16) & 255) / 255.0f;
            fArr2[2] = ((iArr[i] >> 8) & 255) / 255.0f;
            fArr2[3] = (iArr[i] & 255) / 255.0f;
            fArr[i] = fArr2;
        }
        float length = f3 / iArr.length;
        int i2 = 1;
        while (i2 < iArr.length) {
            float f5 = i2 == iArr.length - 1 ? f3 : length * i2;
            trajectoriesVertexConsumer.vertex(matrix4f, f + f5, f2, 0.0f).color(fArr[i2][1], fArr[i2][2], fArr[i2][3], fArr[i2][0]);
            trajectoriesVertexConsumer.vertex(matrix4f, f + (length * (i2 - 1)), f2, 0.0f).color(fArr[i2 - 1][1], fArr[i2 - 1][2], fArr[i2 - 1][3], fArr[i2 - 1][0]);
            trajectoriesVertexConsumer.vertex(matrix4f, f + (length * (i2 - 1)), f2 + f4, 0.0f).color(fArr[i2 - 1][1], fArr[i2 - 1][2], fArr[i2 - 1][3], fArr[i2 - 1][0]);
            trajectoriesVertexConsumer.vertex(matrix4f, f + f5, f2 + f4, 0.0f).color(fArr[i2][1], fArr[i2][2], fArr[i2][3], fArr[i2][0]);
            i2++;
        }
    }

    public static void do1711(MatrixStack matrixStack, String str, int i, int i2, int i3, boolean z) {
        MinecraftClient.getInstance().textRenderer.draw(str, i, i2, i3, z, matrixStack.peek().getPositionMatrix(), FontsSearchHelper4.fontsSearchHelper4.getAdvanceGlyph1686().getVertexConsumerProvider2775(), TextRenderer.TextLayerType.NORMAL, 0, 15728880);
    }

    public static void do1712(MatrixStack matrixStack, OrderedText orderedText, int i, int i2, int i3, boolean z) {
        MinecraftClient.getInstance().textRenderer.draw(orderedText, i, i2, i3, z, matrixStack.peek().getPositionMatrix(), FontsSearchHelper4.fontsSearchHelper4.getAdvanceGlyph1686().getVertexConsumerProvider2775(), TextRenderer.TextLayerType.NORMAL, 0, 15728880);
    }

    public static void do1597() {
        do1713(true);
    }

    public static void do1713(boolean z) {
        RenderSystem.enableBlend();
        RenderSystem.defaultBlendFunc();
        RenderSystem.setShader(GameRenderer::getPositionColorProgram);
        trajectoriesVertexConsumer.do865();
        RenderSystem.disableBlend();
        if (z) {
            FontsSearchHelper4.fontsSearchHelper4.do1597();
        }
    }

    static {
        trajectoriesVertexConsumer.getTrajectoriesVertexConsumer2594(VertexFormat.DrawMode.QUADS, VertexFormats.POSITION_COLOR);
    }
}
