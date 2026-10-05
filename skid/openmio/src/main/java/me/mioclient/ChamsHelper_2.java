package me.mioclient;

import com.mojang.blaze3d.systems.RenderSystem;
import java.awt.Color;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import me.mioclient.MatrixStackEvent;
import me.mioclient.event.Listen;
import net.minecraft.client.model.ModelPart;
import net.minecraft.client.render.GameRenderer;
import net.minecraft.client.render.VertexFormat;
import net.minecraft.client.render.VertexFormats;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.entity.Entity;
import net.minecraft.entity.EntityType;
import net.minecraft.entity.LivingEntity;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import org.joml.Vector4f;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ChamsHelper_2.class */
public class ChamsHelper_2 implements SearchHelper_4 {
    public static final Vector4f vector4f = new Vector4f();
    public static final Vector4f vector4f2 = new Vector4f();
    public static final Vector4f vector4f3 = new Vector4f();
    public static final Vector4f vector4f4 = new Vector4f();
    public static final Map<EntityType<?>, Helper_6<?>> map = new HashMap();
    public static final SearchHelper4_2 searchHelper4_2 = new SearchHelper4_2();
    public static final TrajectoriesVertexConsumer trajectoriesVertexConsumer = TrajectoriesVertexConsumer.getTrajectoriesVertexConsumer2592();
    public static final TrajectoriesVertexConsumer trajectoriesVertexConsumer2 = TrajectoriesVertexConsumer.getTrajectoriesVertexConsumer2592();
    public static Color color;
    public static Color color2;
    public static boolean flag;
    public static double val;

    public ChamsHelper_2() {
        throw new AssertionError();
    }

    @Listen(get219= -9999)
    public static void onEvent(MatrixStackEvent.Inner_3 inner_3) {
        SearchHelper_2.searchHelper_2.do563();
        RenderSystem.setShader(GameRenderer::getPositionColorProgram);
        trajectoriesVertexConsumer.do865();
        RenderSystem.setShader(GameRenderer::getRenderTypeLinesProgram);
        RenderSystem.lineWidth((float) val);
        trajectoriesVertexConsumer2.do865();
        RenderSystem.lineWidth(1.0f);
        SearchHelper_2.searchHelper_2.do565();
    }

    public static void do612(Color color3, Color color4) {
        color = color3;
        color2 = color4;
    }

    public static void do613(boolean z) {
        flag = z;
    }

    public static boolean is614() {
        return flag;
    }

    public static void do615(MatrixStack matrixStack, Entity entity) {
        matrixStack.push();
        Vec3d pos = minecraftClient.gameRenderer.getCamera().getPos();
        float f = SearchHelper_2.get536();
        if (entity instanceof OtherClientPlayerEntity) {
            f = 1.0f;
        }
        double lerp = MathHelper.lerp(f, entity.lastRenderX, entity.getX()) - pos.x;
        double lerp2 = MathHelper.lerp(f, entity.lastRenderY, entity.getY()) - pos.y;
        double lerp3 = MathHelper.lerp(f, entity.lastRenderZ, entity.getZ()) - pos.z;
        Vec3d positionOffset = minecraftClient.getEntityRenderDispatcher().getRenderer(entity).getPositionOffset(entity, f);
        double x = lerp + positionOffset.getX();
        double y = lerp2 + positionOffset.getY();
        double z = lerp3 + positionOffset.getZ();
        matrixStack.push();
        matrixStack.translate(x, y, z);
        Helper_6 helper_6616 = getHelper_6616(entity);
        if (helper_6616 != null) {
            helper_6616.do721(entity, SearchHelper_2.get536(), matrixStack);
        }
        matrixStack.pop();
    }

    public static <E extends Entity> Helper_6<E> getHelper_6616(E e) {
        return (Helper_6) map.getOrDefault(e.getType(), e instanceof LivingEntity ? searchHelper4_2 : null);
    }

    public static void do617(MatrixStackData matrixStackData, ModelPart modelPart) {
        if (modelPart.visible) {
            matrixStackData.getMatrixStack1013().push();
            modelPart.rotate(matrixStackData.getMatrixStack1013());
            Iterator it = modelPart.cuboids.iterator();
            while (it.hasNext()) {
                do618(matrixStackData, (ModelPart.Cuboid) it.next());
            }
            Iterator it2 = modelPart.children.values().iterator();
            while (it2.hasNext()) {
                do617(matrixStackData, (ModelPart) it2.next());
            }
            matrixStackData.getMatrixStack1013().pop();
        }
    }

    public static void do618(MatrixStackData matrixStackData, ModelPart.Cuboid cuboid) {
        for (ModelPart.Quad quad : cuboid.sides) {
            vector4f.set(quad.vertices[0].pos.x / 16.0f, quad.vertices[0].pos.y / 16.0f, quad.vertices[0].pos.z / 16.0f, 1.0f);
            vector4f2.set(quad.vertices[1].pos.x / 16.0f, quad.vertices[1].pos.y / 16.0f, quad.vertices[1].pos.z / 16.0f, 1.0f);
            vector4f3.set(quad.vertices[2].pos.x / 16.0f, quad.vertices[2].pos.y / 16.0f, quad.vertices[2].pos.z / 16.0f, 1.0f);
            vector4f4.set(quad.vertices[3].pos.x / 16.0f, quad.vertices[3].pos.y / 16.0f, quad.vertices[3].pos.z / 16.0f, 1.0f);
            if (color2.getAlpha() != 0) {
                if (!trajectoriesVertexConsumer.is1662()) {
                    trajectoriesVertexConsumer.getBufferBuilder2595(VertexFormat.DrawMode.QUADS, VertexFormats.POSITION_COLOR);
                }
                trajectoriesVertexConsumer.vertex(matrixStackData.getMatrixStack1013().peek().getPositionMatrix(), vector4f.x, vector4f.y, vector4f.z).color(color2.hashCode());
                trajectoriesVertexConsumer.vertex(matrixStackData.getMatrixStack1013().peek().getPositionMatrix(), vector4f2.x, vector4f2.y, vector4f2.z).color(color2.hashCode());
                trajectoriesVertexConsumer.vertex(matrixStackData.getMatrixStack1013().peek().getPositionMatrix(), vector4f3.x, vector4f3.y, vector4f3.z).color(color2.hashCode());
                trajectoriesVertexConsumer.vertex(matrixStackData.getMatrixStack1013().peek().getPositionMatrix(), vector4f4.x, vector4f4.y, vector4f4.z).color(color2.hashCode());
            }
            if (color.getAlpha() != 0) {
                if (!trajectoriesVertexConsumer2.is1662()) {
                    trajectoriesVertexConsumer2.getBufferBuilder2595(VertexFormat.DrawMode.LINES, VertexFormats.LINES);
                }
                do619(matrixStackData, vector4f.x, vector4f.y, vector4f.z, vector4f2.x, vector4f2.y, vector4f2.z);
                do619(matrixStackData, vector4f2.x, vector4f2.y, vector4f2.z, vector4f3.x, vector4f3.y, vector4f3.z);
                do619(matrixStackData, vector4f3.x, vector4f3.y, vector4f3.z, vector4f4.x, vector4f4.y, vector4f4.z);
                do619(matrixStackData, vector4f.x, vector4f.y, vector4f.z, vector4f.x, vector4f.y, vector4f.z);
            }
        }
    }

    public static void do619(MatrixStackData matrixStackData, float f, float f2, float f3, float f4, float f5, float f6) {
        trajectoriesVertexConsumer2.vertex(matrixStackData.getMatrixStack1013().peek().getPositionMatrix(), f, f2, f3).color(color.hashCode()).normal(matrixStackData.getMatrixStack1013().peek(), f4 - f, f5 - f2, f6 - f3);
        trajectoriesVertexConsumer2.vertex(matrixStackData.getMatrixStack1013().peek().getPositionMatrix(), f4, f5, f6).color(color.hashCode()).normal(matrixStackData.getMatrixStack1013().peek(), f4 - f, f5 - f2, f6 - f3);
    }

    public static void do620(double d) {
        val = d;
    }

    static {
        baritoneHelper.do1797(ChamsHelper_2.class);
        map.put(EntityType.END_CRYSTAL, new QuaternionfHelper());
        map.put(EntityType.BOAT, new SearchHelper4_3());
        map.put(EntityType.CHEST_BOAT, new SearchHelper4_3());
        map.put(EntityType.LLAMA, new SearchHelper42());
        map.put(EntityType.RABBIT, new SearchHelper42_2());
        map.put(EntityType.ENDER_DRAGON, new SearchHelper4_16());
        color = Color.white;
        color2 = Color.white;
    }
}
