package me.mioclient;

import com.mojang.blaze3d.systems.RenderSystem;
import it.unimi.dsi.fastutil.objects.ObjectArrayList;
import java.awt.Color;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;
import java.util.Comparator;
import java.util.List;
import me.mioclient.MatrixStackEvent;
import me.mioclient.event.Listen;
import net.minecraft.client.render.GameRenderer;
import net.minecraft.client.render.VertexFormat;
import net.minecraft.client.render.VertexFormats;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;
import org.joml.Matrix3f;
import org.joml.Matrix4f;
import org.lwjgl.opengl.GL20;
import org.lwjgl.opengl.GL32C;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/PhaseESPSearchHelper4.class */
public final class PhaseESPSearchHelper4 implements SearchHelper_4 {
    public static final TrajectoriesVertexConsumer trajectoriesVertexConsumer = TrajectoriesVertexConsumer.getTrajectoriesVertexConsumer2592();
    public static final TrajectoriesVertexConsumer trajectoriesVertexConsumer2 = TrajectoriesVertexConsumer.getTrajectoriesVertexConsumer2592();
    public static final List<Record> list = new ObjectArrayList();

    /* JADX INFO: Access modifiers changed from: package-private */
    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/PhaseESPSearchHelper4$Record.class */
    public static final class Record {
        public final Matrix4f matrix4f;
        public final Matrix3f matrix3f;
        public final Box box;
        public final Color color;
        public final float val;

        public Record(Matrix4f matrix4f, Matrix3f matrix3f, Box box, Color color, float f) {
            this.matrix4f = matrix4f;
            this.matrix3f = matrix3f;
            this.box = box;
            this.color = color;
            this.val = f;
        }

        public static Record getRecord2037(MatrixStack matrixStack, Box box, Color color, float f) {
            return new Record(new Matrix4f(matrixStack.peek().getPositionMatrix()), new Matrix3f(matrixStack.peek().getNormalMatrix()), box, color, f);
        }




        public Matrix4f getMatrix4f2038() {
            return this.matrix4f;
        }

        public Matrix3f getMatrix3f2039() {
            return this.matrix3f;
        }

        public Box getBox799() {
            return this.box;
        }

        public Color getColor2040() {
            return this.color;
        }

        public float get2041() {
            return this.val;
        }
    }

    @Listen(get219= -9999)
    public static void onEvent(MatrixStackEvent.Inner_3 inner_3) {
        do1597();
        RenderSystem.enablePolygonOffset();
        RenderSystem.polygonOffset(1.0f, -1500000.0f);
        GL20.glDepthRange(0.0d, 0.1d);
        CrosshairHelper.do1597();
        GL20.glDepthRange(0.0d, 1.0d);
        RenderSystem.polygonOffset(1.0f, 1500000.0f);
        RenderSystem.disablePolygonOffset();
    }

    public static void do1589(MatrixStack matrixStack, BlockPos blockPos, Color color) {
        do1590(matrixStack, new Box(blockPos), color);
    }

    public static void do1590(MatrixStack matrixStack, Box box, Color color) {
        if (color.getAlpha() == 0) {
            return;
        }
        float x = (float) (box.minX - minecraftClient.getEntityRenderDispatcher().camera.getPos().getX());
        float y = (float) (box.minY - minecraftClient.getEntityRenderDispatcher().camera.getPos().getY());
        float z = (float) (box.minZ - minecraftClient.getEntityRenderDispatcher().camera.getPos().getZ());
        float x2 = (float) (box.maxX - minecraftClient.getEntityRenderDispatcher().camera.getPos().getX());
        float y2 = (float) (box.maxY - minecraftClient.getEntityRenderDispatcher().camera.getPos().getY());
        float z2 = (float) (box.maxZ - minecraftClient.getEntityRenderDispatcher().camera.getPos().getZ());
        int hashCode = getColor1595(color).hashCode();
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y, z).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y, z).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y, z2).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y, z2).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y2, z).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y2, z).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y2, z2).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y2, z2).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y, z).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y2, z).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y2, z).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y, z).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y, z).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y2, z).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y2, z2).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y, z2).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y, z2).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y, z2).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y2, z2).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y2, z2).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y, z).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y, z2).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y2, z2).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y2, z).color(hashCode);
    }

    public static void do1591(MatrixStack matrixStack, Box box, Color color) {
        if (color.getAlpha() == 0) {
            return;
        }
        float x = (float) (box.minX - minecraftClient.getEntityRenderDispatcher().camera.getPos().getX());
        float y = (float) (box.minY - minecraftClient.getEntityRenderDispatcher().camera.getPos().getY());
        float z = (float) (box.minZ - minecraftClient.getEntityRenderDispatcher().camera.getPos().getZ());
        float x2 = (float) (box.maxX - minecraftClient.getEntityRenderDispatcher().camera.getPos().getX());
        float y2 = (float) (box.maxY - minecraftClient.getEntityRenderDispatcher().camera.getPos().getY());
        float z2 = (float) (box.maxZ - minecraftClient.getEntityRenderDispatcher().camera.getPos().getZ());
        int hashCode = color.hashCode();
        int i = MixinMessageIndicatorHelper_2.get818(color, 0);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y, z).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y, z).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y, z2).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y, z2).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y, z).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y2, z).color(i);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y2, z).color(i);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y, z).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y, z).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y2, z).color(i);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y2, z2).color(i);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y, z2).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y, z2).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y, z2).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x2, y2, z2).color(i);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y2, z2).color(i);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y, z).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y, z2).color(hashCode);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y2, z2).color(i);
        trajectoriesVertexConsumer.vertex(matrixStack.peek().getPositionMatrix(), x, y2, z).color(i);
    }

    public static void do1592(MatrixStack matrixStack, BlockPos blockPos, Color color, float f) {
        do1593(matrixStack, new Box(blockPos), color, f);
    }

    public static void do1593(MatrixStack matrixStack, Box box, Color color, float f) {
        if (color.getAlpha() != 0 && f > 0.0f) {
            list.add(Record.getRecord2037(matrixStack, box, getColor1595(color), f));
        }
    }

    public static void do1594(Record record) {
        Matrix4f matrix4f = record.matrix4f;
        Matrix3f matrix3f = record.matrix3f;
        float x = (float) (record.box.minX - minecraftClient.getEntityRenderDispatcher().camera.getPos().getX());
        float y = (float) (record.box.minY - minecraftClient.getEntityRenderDispatcher().camera.getPos().getY());
        float z = (float) (record.box.minZ - minecraftClient.getEntityRenderDispatcher().camera.getPos().getZ());
        float x2 = (float) (record.box.maxX - minecraftClient.getEntityRenderDispatcher().camera.getPos().getX());
        float y2 = (float) (record.box.maxY - minecraftClient.getEntityRenderDispatcher().camera.getPos().getY());
        float z2 = (float) (record.box.maxZ - minecraftClient.getEntityRenderDispatcher().camera.getPos().getZ());
        int hashCode = record.color.hashCode();
        trajectoriesVertexConsumer2.vertex(matrix4f, x, y, z).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x2, y, z).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x, y, z).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x, y2, z).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x, y, z).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x, y, z2).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x2, y, z).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x2, y2, z).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x2, y2, z).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x, y2, z).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x, y2, z).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x, y2, z2).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x, y2, z2).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x, y, z2).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x, y, z2).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x2, y, z2).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x2, y, z2).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x2, y, z).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x, y2, z2).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x2, y2, z2).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x2, y, z2).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x2, y2, z2).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x2, y2, z).color(hashCode);
        trajectoriesVertexConsumer2.vertex(matrix4f, x2, y2, z2).color(hashCode);
    }

    public static Color getColor1595(Color color) {
        return getColor1596(color, RenderSystem.getShaderColor());
    }

    public static Color getColor1596(Color color, float[] fArr) {
        return new Color((int) (color.getRed() * fArr[0]), (int) (color.getGreen() * fArr[1]), (int) (color.getBlue() * fArr[2]), (int) (color.getAlpha() * fArr[3]));
    }

    public static void do1597() {
        SearchHelper_2.searchHelper_2.do563();
        RenderSystem.setShader(GameRenderer::getPositionColorProgram);
        trajectoriesVertexConsumer.do865();
        list.sort(Comparator.comparing(record -> {
            return Float.valueOf(record.val);
        }));
        float f = 0.0f;
        for (Record record2 : list) {
            if (record2.val != f) {
                do1599();
            }
            f = record2.val;
            do1598(f);
            do1594(record2);
        }
        list.clear();
        do1599();
        SearchHelper_2.searchHelper_2.do565();
    }

    public static void do1598(float f) {
        GL32C.glLineWidth(f);
    }

    public static void do1599() {
        trajectoriesVertexConsumer2.do865();
        GL32C.glLineWidth(1.0f);
    }

    static {
        baritoneHelper.do1797(PhaseESPSearchHelper4.class);
        trajectoriesVertexConsumer.getTrajectoriesVertexConsumer2594(VertexFormat.DrawMode.QUADS, VertexFormats.POSITION_COLOR);
        trajectoriesVertexConsumer2.getTrajectoriesVertexConsumer2594(VertexFormat.DrawMode.DEBUG_LINES, VertexFormats.POSITION_COLOR);
    }
}
