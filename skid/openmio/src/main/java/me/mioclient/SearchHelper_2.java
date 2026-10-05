package me.mioclient;

import com.mojang.blaze3d.systems.RenderSystem;
import java.awt.Color;
import me.mioclient.module.client.UI;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.render.BufferBuilder;
import net.minecraft.client.render.BufferRenderer;
import net.minecraft.client.render.Camera;
import net.minecraft.client.render.GameRenderer;
import net.minecraft.client.render.Tessellator;
import net.minecraft.client.render.VertexConsumer;
import net.minecraft.client.render.VertexConsumerProvider;
import net.minecraft.client.render.VertexFormat;
import net.minecraft.client.render.VertexFormats;
import net.minecraft.client.render.WorldRenderer;
import net.minecraft.client.render.entity.EntityRenderDispatcher;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.entity.Entity;
import net.minecraft.text.Text;
import net.minecraft.util.Identifier;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.ColorHelper;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.RotationAxis;
import net.minecraft.util.math.Vec3d;
import org.joml.Matrix4f;
import org.lwjgl.opengl.GL20;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper_2.class */
public class SearchHelper_2 implements SearchHelper_4 {
    public static final SearchHelper_2 searchHelper_2 = new SearchHelper_2();
    public static float val;
    public static MatrixStack matrixStack;

    public static float get536() {
        return val;
    }

    public static void do537(float f) {
        val = f;
    }

    public static MatrixStack getMatrixStack472() {
        return matrixStack;
    }

    public static void do538(MatrixStack matrixStack2) {
        matrixStack = matrixStack2;
    }

    public void do539(MatrixStack matrixStack2, float f, float f2, float f3, float f4, Color color) {
        if (f3 < f) {
            f = f3;
            f3 = f;
        }
        if (f4 < f2) {
            f2 = f4;
            f4 = f2;
        }
        do541(matrixStack2, f, f3, f2, color);
        do542(matrixStack2, f3, f2, f4, color);
        do541(matrixStack2, f, f3, f4, color);
        do542(matrixStack2, f, f2, f4, color);
    }

    public void do540(MatrixStack matrixStack2, float f, float f2, float f3, float f4, Color color, float f5) {
        do543(matrixStack2, f, f3, f2, color, f5);
        do544(matrixStack2, f3, f2, f4, color, f5);
        do543(matrixStack2, f, f3, f4, color, f5);
        do544(matrixStack2, f, f2, f4, color, f5);
    }

    public void do541(MatrixStack matrixStack2, float f, float f2, float f3, Color color) {
        if (f2 < f) {
            f = f2;
            f2 = f;
        }
        do546(matrixStack2, f, f3, f2 + 1.0f, f3 + 1.0f, color);
    }

    public void do542(MatrixStack matrixStack2, float f, float f2, float f3, Color color) {
        if (f3 < f2) {
            f2 = f3;
            f3 = f2;
        }
        do546(matrixStack2, f, f2 + 1.0f, f + 1.0f, f3, color);
    }

    public void do543(MatrixStack matrixStack2, float f, float f2, float f3, Color color, float f4) {
        if (f2 < f) {
            f = f2;
            f2 = f;
        }
        do546(matrixStack2, f, f3, f2 + f4, f3 + f4, color);
    }

    public void do544(MatrixStack matrixStack2, float f, float f2, float f3, Color color, float f4) {
        if (f3 < f2) {
            f2 = f3;
            f3 = f2;
        }
        do546(matrixStack2, f, f2 + f4, f + f4, f3, color);
    }

    public void do545(MatrixStack matrixStack2, float f, float f2, float f3, float f4, int i) {
        if (f < f3) {
            f = f3;
            f3 = f;
        }
        if (f2 < f4) {
            f2 = f4;
            f4 = f2;
        }
        float f5 = ((i >> 24) & 255) / 255.0f;
        float f6 = ((i >> 16) & 255) / 255.0f;
        float f7 = ((i >> 8) & 255) / 255.0f;
        float f8 = (i & 255) / 255.0f;
        RenderSystem.enableBlend();
        RenderSystem.defaultBlendFunc();
        RenderSystem.setShader(GameRenderer::getPositionColorProgram);
        BufferBuilder begin = Tessellator.getInstance().begin(VertexFormat.DrawMode.QUADS, VertexFormats.POSITION_COLOR);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), f, f4, 0.0f).color(f6, f7, f8, f5);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), f3, f4, 0.0f).color(f6, f7, f8, f5);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), f3, f2, 0.0f).color(f6, f7, f8, f5);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), f, f2, 0.0f).color(f6, f7, f8, f5);
        BufferRenderer.drawWithGlobalProgram(begin.end());
        RenderSystem.disableBlend();
    }

    public void do546(MatrixStack matrixStack2, float f, float f2, float f3, float f4, Color color) {
        do545(matrixStack2, f, f2, f3, f4, color.hashCode());
    }

    public void do547(MatrixStack matrixStack2, int i, int i2, int i3, int i4, Color color, Color color2) {
        RenderSystem.enableBlend();
        RenderSystem.setShader(GameRenderer::getPositionColorProgram);
        Tessellator.getInstance();
        BufferBuilder begin = Tessellator.getInstance().begin(VertexFormat.DrawMode.QUADS, VertexFormats.POSITION_COLOR);
        do548(matrixStack2.peek().getPositionMatrix(), begin, i, i2, i3, i4, 0, color.hashCode(), color2.hashCode());
        BufferRenderer.drawWithGlobalProgram(begin.end());
        RenderSystem.disableBlend();
    }

    public void do548(Matrix4f matrix4f, VertexConsumer vertexConsumer, int i, int i2, int i3, int i4, int i5, int i6, int i7) {
        float alpha = ColorHelper.Argb.getAlpha(i6) / 255.0f;
        float red = ColorHelper.Argb.getRed(i6) / 255.0f;
        float green = ColorHelper.Argb.getGreen(i6) / 255.0f;
        float blue = ColorHelper.Argb.getBlue(i6) / 255.0f;
        float alpha2 = ColorHelper.Argb.getAlpha(i7) / 255.0f;
        float red2 = ColorHelper.Argb.getRed(i7) / 255.0f;
        float green2 = ColorHelper.Argb.getGreen(i7) / 255.0f;
        float blue2 = ColorHelper.Argb.getBlue(i7) / 255.0f;
        vertexConsumer.vertex(matrix4f, i, i2, i5).color(red, green, blue, alpha);
        vertexConsumer.vertex(matrix4f, i, i4, i5).color(red2, green2, blue2, alpha2);
        vertexConsumer.vertex(matrix4f, i3, i4, i5).color(red2, green2, blue2, alpha2);
        vertexConsumer.vertex(matrix4f, i3, i2, i5).color(red, green, blue, alpha);
    }

    public int get549(String str) {
        return minecraftClient.textRenderer.getWidth(str);
    }

    public int get550(Text text) {
        return minecraftClient.textRenderer.getWidth(text);
    }

    public double get551() {
        return minecraftClient.getWindow().getScaledWidth();
    }

    public double get552() {
        return minecraftClient.getWindow().getScaledHeight();
    }

    public void do553(MatrixStack matrixStack2, Box box, Color color) {
        float x = (float) (box.minX - minecraftClient.getEntityRenderDispatcher().camera.getPos().getX());
        float y = (float) (box.minY - minecraftClient.getEntityRenderDispatcher().camera.getPos().getY());
        float z = (float) (box.minZ - minecraftClient.getEntityRenderDispatcher().camera.getPos().getZ());
        float x2 = (float) (box.maxX - minecraftClient.getEntityRenderDispatcher().camera.getPos().getX());
        float y2 = (float) (box.maxY - minecraftClient.getEntityRenderDispatcher().camera.getPos().getY());
        float z2 = (float) (box.maxZ - minecraftClient.getEntityRenderDispatcher().camera.getPos().getZ());
        do563();
        RenderSystem.setShader(GameRenderer::getPositionColorProgram);
        BufferBuilder begin = Tessellator.getInstance().begin(VertexFormat.DrawMode.QUADS, VertexFormats.POSITION_COLOR);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y, z).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y, z).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y, z2).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y, z2).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y2, z).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y2, z2).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y2, z2).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y2, z).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y, z).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y2, z).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y2, z).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y, z).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y, z).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y2, z).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y2, z2).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y, z2).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y, z2).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y, z2).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y2, z2).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y2, z2).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y, z).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y, z2).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y2, z2).color(color.getRGB());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y2, z).color(color.getRGB());
        BufferRenderer.drawWithGlobalProgram(begin.end());
        do565();
    }

    public void do554(MatrixStack matrixStack2, Box box, Color color) {
        float x = (float) (box.minX - minecraftClient.getEntityRenderDispatcher().camera.getPos().getX());
        float y = (float) (box.minY - minecraftClient.getEntityRenderDispatcher().camera.getPos().getY());
        float z = (float) (box.minZ - minecraftClient.getEntityRenderDispatcher().camera.getPos().getZ());
        float x2 = (float) (box.maxX - minecraftClient.getEntityRenderDispatcher().camera.getPos().getX());
        float y2 = (float) (box.maxY - minecraftClient.getEntityRenderDispatcher().camera.getPos().getY());
        float z2 = (float) (box.maxZ - minecraftClient.getEntityRenderDispatcher().camera.getPos().getZ());
        int hashCode = color.hashCode();
        int i = MixinMessageIndicatorHelper_2.get818(color, 0);
        do563();
        RenderSystem.setShader(GameRenderer::getPositionColorProgram);
        BufferBuilder begin = Tessellator.getInstance().begin(VertexFormat.DrawMode.QUADS, VertexFormats.POSITION_COLOR);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y, z).color(hashCode);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y, z).color(hashCode);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y, z2).color(hashCode);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y, z2).color(hashCode);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y, z).color(hashCode);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y2, z).color(i);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y2, z).color(i);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y, z).color(hashCode);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y, z).color(hashCode);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y2, z).color(i);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y2, z2).color(i);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y, z2).color(hashCode);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y, z2).color(hashCode);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y, z2).color(hashCode);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y2, z2).color(i);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y2, z2).color(i);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y, z).color(hashCode);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y, z2).color(hashCode);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y2, z2).color(i);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y2, z).color(i);
        BufferRenderer.drawWithGlobalProgram(begin.end());
        do565();
    }

    public void do555(MatrixStack matrixStack2, Vec3d vec3d, Color color) {
        do553(matrixStack2, Box.from(vec3d), color);
    }

    public void do556(MatrixStack matrixStack2, BlockPos blockPos, Color color) {
        do553(matrixStack2, new Box(blockPos), color);
    }

    public void do557(MatrixStack matrixStack2, Box box, Color color, double d) {
        float x = (float) (box.minX - minecraftClient.getEntityRenderDispatcher().camera.getPos().getX());
        float y = (float) (box.minY - minecraftClient.getEntityRenderDispatcher().camera.getPos().getY());
        float z = (float) (box.minZ - minecraftClient.getEntityRenderDispatcher().camera.getPos().getZ());
        float x2 = (float) (box.maxX - minecraftClient.getEntityRenderDispatcher().camera.getPos().getX());
        float y2 = (float) (box.maxY - minecraftClient.getEntityRenderDispatcher().camera.getPos().getY());
        float z2 = (float) (box.maxZ - minecraftClient.getEntityRenderDispatcher().camera.getPos().getZ());
        Tessellator.getInstance();
        do563();
        RenderSystem.lineWidth((float) d);
        RenderSystem.setShader(GameRenderer::getRenderTypeLinesProgram);
        GL20.glEnable(2848);
        GL20.glHint(3154, 4354);
        RenderSystem.defaultBlendFunc();
        BufferBuilder begin = Tessellator.getInstance().begin(VertexFormat.DrawMode.LINES, VertexFormats.LINES);
        WorldRenderer.drawBox(matrixStack2, begin, x, y, z, x2, y2, z2, color.getRed() / 255.0f, color.getGreen() / 255.0f, color.getBlue() / 255.0f, color.getAlpha() / 255.0f);
        BufferRenderer.drawWithGlobalProgram(begin.end());
        GL20.glDisable(2848);
        do565();
    }

    public void do558(MatrixStack matrixStack2, Vec3d vec3d, Color color, double d) {
        do557(matrixStack2, Box.from(vec3d), color, d);
    }

    public void do559(MatrixStack matrixStack2, BlockPos blockPos, Color color, double d) {
        do557(matrixStack2, new Box(blockPos), color, d);
    }

    public void do560(MatrixStack matrixStack2, Vec3d vec3d, Vec3d vec3d2, Color color, float f) {
        float x = (float) (vec3d.x - minecraftClient.getEntityRenderDispatcher().camera.getPos().getX());
        float y = (float) (vec3d.y - minecraftClient.getEntityRenderDispatcher().camera.getPos().getY());
        float z = (float) (vec3d.z - minecraftClient.getEntityRenderDispatcher().camera.getPos().getZ());
        float x2 = (float) (vec3d2.x - minecraftClient.getEntityRenderDispatcher().camera.getPos().getX());
        float y2 = (float) (vec3d2.y - minecraftClient.getEntityRenderDispatcher().camera.getPos().getY());
        float z2 = (float) (vec3d2.z - minecraftClient.getEntityRenderDispatcher().camera.getPos().getZ());
        MatrixStack.Entry peek = matrixStack2.peek();
        do563();
        RenderSystem.lineWidth(f);
        RenderSystem.setShader(GameRenderer::getRenderTypeLinesProgram);
        GL20.glEnable(2848);
        GL20.glHint(3154, 4354);
        RenderSystem.defaultBlendFunc();
        BufferBuilder begin = Tessellator.getInstance().begin(VertexFormat.DrawMode.QUADS, VertexFormats.POSITION_COLOR);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y, z).color(color.getRGB()).normal(peek, f, 0.0f, 0.0f);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y2, z2).color(color.getRGB()).normal(peek, f, 0.0f, 0.0f);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y, z).color(color.getRGB()).normal(peek, 0.0f, f, 0.0f);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y2, z2).color(color.getRGB()).normal(peek, 0.0f, f, 0.0f);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y, z).color(color.getRGB()).normal(peek, 0.0f, 0.0f, f);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y2, z2).color(color.getRGB()).normal(peek, 0.0f, 0.0f, f);
        BufferRenderer.drawWithGlobalProgram(begin.end());
        GL20.glDisable(2848);
        do565();
    }

    public void do561(MatrixStack matrixStack2, Vec3d vec3d, Vec3d vec3d2, Color color) {
        float x = (float) (vec3d.x - minecraftClient.getEntityRenderDispatcher().camera.getPos().getX());
        float y = (float) (vec3d.y - minecraftClient.getEntityRenderDispatcher().camera.getPos().getY());
        float z = (float) (vec3d.z - minecraftClient.getEntityRenderDispatcher().camera.getPos().getZ());
        float x2 = (float) (vec3d2.x - minecraftClient.getEntityRenderDispatcher().camera.getPos().getX());
        float y2 = (float) (vec3d2.y - minecraftClient.getEntityRenderDispatcher().camera.getPos().getY());
        float z2 = (float) (vec3d2.z - minecraftClient.getEntityRenderDispatcher().camera.getPos().getZ());
        RenderSystem.setShader(GameRenderer::getPositionColorProgram);
        searchHelper_2.do563();
        BufferBuilder begin = Tessellator.getInstance().begin(VertexFormat.DrawMode.DEBUG_LINES, VertexFormats.POSITION_COLOR);
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x, y, z).color(color.hashCode());
        begin.vertex(matrixStack2.peek().getPositionMatrix(), x2, y2, z2).color(color.hashCode());
        BufferRenderer.drawWithGlobalProgram(begin.end());
        searchHelper_2.do565();
    }

    public void do562() {
        RenderSystem.enableBlend();
        RenderSystem.defaultBlendFunc();
    }

    public void do563() {
        do562();
        RenderSystem.disableDepthTest();
        RenderSystem.depthMask(false);
        RenderSystem.disableCull();
    }

    public void do564() {
        RenderSystem.disableBlend();
    }

    public void do565() {
        do564();
        RenderSystem.enableDepthTest();
        RenderSystem.depthMask(true);
        RenderSystem.enableCull();
    }

    public void do566(java.lang.Runnable runnable) {
        if (UI.uI.msaa.getValue() == UI.Mode.NONE) {
            runnable.run();
        } else {
            int i = UI.uI.msaa.getValue().get150();
            Framebuffer.do2805((i & (i - 1)) == 0 ? i : 2, runnable);
        }
    }

    public void do567(MatrixStack matrixStack2, Vec3d vec3d, float f, float f2, float f3, float f4, double d, Color color) {
        matrixStack2.push();
        do577(matrixStack2, vec3d);
        RenderSystem.defaultBlendFunc();
        matrixStack2.translate(f, f2, 0.0f);
        matrixStack2.multiply(minecraftClient.getEntityRenderDispatcher().getRotation());
        matrixStack2.scale(0.025f * ((float) d), (-0.025f) * ((float) d), (float) (0.02500000037252903d * d));
        int i = (int) (f3 / 2.0f);
        RenderSystem.disableDepthTest();
        do546(matrixStack2, -i, 0.0f, (int) (f3 - i), (int) f4, color);
        RenderSystem.enableDepthTest();
        matrixStack2.pop();
    }

    public void do568(DrawContext drawContext, Vec3d vec3d, float f, float f2, double d, Color color, Identifier identifier) {
        MatrixStack matrices = drawContext.getMatrices();
        matrices.push();
        do577(matrices, vec3d);
        Camera camera = minecraftClient.gameRenderer.getCamera();
        matrices.multiply(RotationAxis.POSITIVE_Y.rotationDegrees(-camera.getYaw()));
        matrices.multiply(RotationAxis.POSITIVE_X.rotationDegrees(camera.getPitch()));
        RenderSystem.defaultBlendFunc();
        matrices.scale(0.025f * ((float) d), (-0.025f) * ((float) d), 1.0f);
        int i = (int) (f / 2.0f);
        RenderSystem.disableDepthTest();
        RenderSystem.disableCull();
        RenderSystem.setShader(GameRenderer::getPositionTexColorProgram);
        RenderSystem.setShaderColor(color.getRed() / 255.0f, color.getGreen() / 255.0f, color.getBlue() / 255.0f, 1.0f);
        drawContext.drawTexture(identifier, -i, -i, 0, 0, (int) f, (int) f2);
        RenderSystem.setShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
        RenderSystem.enableDepthTest();
        matrices.pop();
    }

    public void do569(MatrixStack matrixStack2, Vec3d vec3d, float f, float f2, float f3, float f4, double d, Color color) {
        matrixStack2.push();
        do577(matrixStack2, vec3d);
        Camera camera = minecraftClient.gameRenderer.getCamera();
        matrixStack2.multiply(RotationAxis.POSITIVE_Y.rotationDegrees(-camera.getYaw()));
        matrixStack2.multiply(RotationAxis.POSITIVE_X.rotationDegrees(camera.getPitch()));
        RenderSystem.defaultBlendFunc();
        matrixStack2.translate(f, f2, 0.0f);
        matrixStack2.scale(0.025f * ((float) d), (-0.025f) * ((float) d), 1.0f);
        int i = (int) (f3 / 2.0f);
        RenderSystem.disableDepthTest();
        do539(matrixStack2, -i, 0.0f, (int) (f3 - i), (int) f4, color);
        RenderSystem.enableDepthTest();
        matrixStack2.pop();
    }

    public void do570(DrawContext drawContext, String str, Vec3d vec3d, float f, float f2, double d, Color color, boolean z) {
        do571(drawContext, str, vec3d, f, f2, (-FontsSearchHelper4.fontsSearchHelper4.get1316(str)) / 2.0f, 0.0f, d, color, z);
    }

    public void do571(DrawContext drawContext, String str, Vec3d vec3d, float f, float f2, float f3, float f4, double d, Color color, boolean z) {
        drawContext.getMatrices().push();
        do577(drawContext.getMatrices(), vec3d);
        RenderSystem.defaultBlendFunc();
        drawContext.getMatrices().translate(f, f2, 0.0f);
        drawContext.getMatrices().multiply(minecraftClient.getEntityRenderDispatcher().getRotation());
        drawContext.getMatrices().scale(0.025f * ((float) d), (-0.025f) * ((float) d), (float) (0.02500000037252903d * d));
        if (z) {
            FontsSearchHelper4.fontsSearchHelper4.do1691(drawContext, str, f3, f4, color);
        } else {
            FontsSearchHelper4.fontsSearchHelper4.do1689(drawContext, str, f3, f4, color);
        }
        drawContext.getMatrices().pop();
    }

    public void do572(MatrixStack matrixStack2, BufferBuilder bufferBuilder, Vec3d vec3d, Vec3d vec3d2, int i, int i2) {
        float x = (float) (vec3d.getX() - minecraftClient.getEntityRenderDispatcher().camera.getPos().getX());
        float y = (float) (vec3d.getY() - minecraftClient.getEntityRenderDispatcher().camera.getPos().getY());
        float z = (float) (vec3d.getZ() - minecraftClient.getEntityRenderDispatcher().camera.getPos().getZ());
        float x2 = (float) (vec3d2.getX() - minecraftClient.getEntityRenderDispatcher().camera.getPos().getX());
        float y2 = (float) (vec3d2.getY() - minecraftClient.getEntityRenderDispatcher().camera.getPos().getY());
        float z2 = (float) (vec3d2.getZ() - minecraftClient.getEntityRenderDispatcher().camera.getPos().getZ());
        bufferBuilder.vertex(matrixStack2.peek().getPositionMatrix(), x, y, z).color(i).normal(matrixStack2.peek(), x2 - x, y2 - y, z2 - z);
        bufferBuilder.vertex(matrixStack2.peek().getPositionMatrix(), x2, y2, z2).color(i2).normal(matrixStack2.peek(), x2 - x, y2 - y, z2 - z);
    }

    public void do573(MatrixStack matrixStack2, BufferBuilder bufferBuilder, Box box, int i) {
        float x = (float) (box.minX - minecraftClient.getEntityRenderDispatcher().camera.getPos().getX());
        float y = (float) (box.minY - minecraftClient.getEntityRenderDispatcher().camera.getPos().getY());
        float z = (float) (box.minZ - minecraftClient.getEntityRenderDispatcher().camera.getPos().getZ());
        float x2 = (float) (box.maxX - minecraftClient.getEntityRenderDispatcher().camera.getPos().getX());
        float z2 = (float) (box.maxZ - minecraftClient.getEntityRenderDispatcher().camera.getPos().getZ());
        Matrix4f positionMatrix = matrixStack2.peek().getPositionMatrix();
        bufferBuilder.vertex(positionMatrix, x, y, z).color(i);
        bufferBuilder.vertex(positionMatrix, x2, y, z).color(i);
        bufferBuilder.vertex(positionMatrix, x2, y, z2).color(i);
        bufferBuilder.vertex(positionMatrix, x, y, z2).color(i);
        bufferBuilder.vertex(positionMatrix, x, y, z).color(i);
    }

    public void do574(Entity entity, float f, MatrixStack matrixStack2, VertexConsumerProvider vertexConsumerProvider) {
        EntityRenderDispatcher entityRenderDispatcher = minecraftClient.getEntityRenderDispatcher();
        Vec3d pos = minecraftClient.gameRenderer.getCamera().getPos();
        entityRenderDispatcher.render(entity, MathHelper.lerp(f, entity.lastRenderX, entity.getX()) - pos.x, MathHelper.lerp(f, entity.lastRenderY, entity.getY()) - pos.y, MathHelper.lerp(f, entity.lastRenderZ, entity.getZ()) - pos.z, MathHelper.lerp(f, entity.prevYaw, entity.getYaw()), f, matrixStack2, vertexConsumerProvider, entityRenderDispatcher.getLight(entity, f));
    }

    public void do575(Matrix4f matrix4f, float f, float f2, float f3, float f4, int i, int i2, int i3, int i4) {
        float[] fArr = {((i >> 16) & 255) / 255.0f, ((i >> 8) & 255) / 255.0f, (i & 255) / 255.0f};
        float[] fArr2 = {((i2 >> 16) & 255) / 255.0f, ((i2 >> 8) & 255) / 255.0f, (i2 & 255) / 255.0f};
        float[] fArr3 = {((i3 >> 16) & 255) / 255.0f, ((i3 >> 8) & 255) / 255.0f, (i3 & 255) / 255.0f};
        float[] fArr4 = {((i4 >> 16) & 255) / 255.0f, ((i4 >> 8) & 255) / 255.0f, (i4 & 255) / 255.0f};
        do562();
        RenderSystem.setShader(GameRenderer::getPositionColorProgram);
        BufferBuilder begin = Tessellator.getInstance().begin(VertexFormat.DrawMode.QUADS, VertexFormats.POSITION_COLOR);
        begin.vertex(matrix4f, f, f2 + f4, 0.0f).color(fArr4[0], fArr4[1], fArr4[2], 1.0f);
        begin.vertex(matrix4f, f + f3, f2 + f4, 0.0f).color(fArr3[0], fArr3[1], fArr3[2], 1.0f);
        begin.vertex(matrix4f, f + f3, f2, 0.0f).color(fArr2[0], fArr2[1], fArr2[2], 1.0f);
        begin.vertex(matrix4f, f, f2, 0.0f).color(fArr[0], fArr[1], fArr[2], 1.0f);
        BufferRenderer.drawWithGlobalProgram(begin.end());
        do564();
    }

    public void do576(Matrix4f matrix4f, float f, float f2, float f3, float f4, int... iArr) {
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
        do562();
        RenderSystem.setShader(GameRenderer::getPositionColorProgram);
        float length = f3 / iArr.length;
        int i2 = 1;
        while (i2 < iArr.length) {
            Tessellator.getInstance();
            BufferBuilder begin = Tessellator.getInstance().begin(VertexFormat.DrawMode.QUADS, VertexFormats.POSITION_COLOR);
            begin.vertex(matrix4f, f + (i2 == iArr.length - 1 ? f3 : length * i2), f2, 0.0f).color(fArr[i2][1], fArr[i2][2], fArr[i2][3], fArr[i2][0]);
            begin.vertex(matrix4f, f + (length * (i2 - 1)), f2, 0.0f).color(fArr[i2 - 1][1], fArr[i2 - 1][2], fArr[i2 - 1][3], fArr[i2 - 1][0]);
            begin.vertex(matrix4f, f + (length * (i2 - 1)), f2 + f4, 0.0f).color(fArr[i2 - 1][1], fArr[i2 - 1][2], fArr[i2 - 1][3], fArr[i2 - 1][0]);
            begin.vertex(matrix4f, f + (i2 == iArr.length - 1 ? f3 : length * i2), f2 + f4, 0.0f).color(fArr[i2][1], fArr[i2][2], fArr[i2][3], fArr[i2][0]);
            BufferRenderer.drawWithGlobalProgram(begin.end());
            i2++;
        }
        do564();
    }

    public static void do577(MatrixStack matrixStack2, Vec3d vec3d) {
        Vec3d pos = minecraftClient.gameRenderer.getCamera().getPos();
        matrixStack2.translate(vec3d.x - pos.x, vec3d.y - pos.y, vec3d.z - pos.z);
    }

    public static void do578(MatrixStack matrixStack2) {
        Camera camera = minecraftClient.gameRenderer.getCamera();
        matrixStack2.multiply(RotationAxis.POSITIVE_X.rotationDegrees(camera.getPitch()));
        matrixStack2.multiply(RotationAxis.POSITIVE_Y.rotationDegrees(camera.getYaw() + 180.0f));
    }
}
