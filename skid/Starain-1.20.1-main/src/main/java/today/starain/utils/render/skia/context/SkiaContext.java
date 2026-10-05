package today.starain.utils.render.skia.context;

import com.mojang.blaze3d.platform.GlConst;
import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.blaze3d.vertex.BufferUploader;
import io.github.humbleui.skija.*;
import lombok.Getter;
import net.minecraft.client.Minecraft;
import org.lwjgl.opengl.GL11;
import org.lwjgl.opengl.GL13;
import org.lwjgl.opengl.GL33;

import java.util.function.Consumer;

public class SkiaContext {

    @Getter
    private static DirectContext context = null;
    private static Surface surface;
    private static BackendRenderTarget renderTarget;

    public static Canvas getCanvas() {
        return surface.getCanvas();
    }

    public static void createSurface(int width, int height) {
        System.out.println("createSurface");
        if (context == null) {
            context = DirectContext.makeGL();
        }

        if (surface != null) {
            surface.close();
            surface = null;
        }

        if (renderTarget != null) {
            renderTarget.close();
            renderTarget = null;
        }

        renderTarget = BackendRenderTarget.makeGL(width, height, 0, 8,
                Minecraft.getInstance().getMainRenderTarget().frameBufferId, GL11.GL_RGBA8);
        surface = Surface.wrapBackendRenderTarget(context, renderTarget, SurfaceOrigin.BOTTOM_LEFT,
                SurfaceColorFormat.RGBA_8888, ColorSpace.getSRGB());
    }

    public static void draw(Consumer<Canvas> drawingLogic) {

        RenderSystem.pixelStore(GlConst.GL_UNPACK_ROW_LENGTH, 0);
        RenderSystem.pixelStore(GlConst.GL_UNPACK_SKIP_PIXELS, 0);
        RenderSystem.pixelStore(GlConst.GL_UNPACK_SKIP_ROWS, 0);
        RenderSystem.pixelStore(GlConst.GL_UNPACK_ALIGNMENT, 4);
        RenderSystem.clearColor(0f, 0f, 0f, 0f);
        context.resetGLAll();

        Canvas canvas = getCanvas();
        drawingLogic.accept(canvas);

        context.flush();

        BufferUploader.reset();
        GL33.glBindSampler(0, 0);
        RenderSystem.disableBlend();
        GL11.glDisable(GL11.GL_BLEND);
        // 还原为默认 alpha 混合（原来留成 SRC_ALPHA/ONE 加法混合会让后续世界渲染偏亮）
        RenderSystem.blendFunc(GL11.GL_SRC_ALPHA, GL11.GL_ONE_MINUS_SRC_ALPHA);
        GL11.glBlendFunc(GL11.GL_SRC_ALPHA, GL11.GL_ONE_MINUS_SRC_ALPHA);
        RenderSystem.blendEquation(GL33.GL_FUNC_ADD);
        GL33.glBlendEquation(GL33.GL_FUNC_ADD);
        RenderSystem.colorMask(true, true, true, true);
        GL11.glColorMask(true, true, true, true);
        RenderSystem.depthMask(true);
        GL11.glDepthMask(true);
        RenderSystem.disableScissor();
        GL11.glDisable(GL11.GL_SCISSOR_TEST);
        GL11.glDisable(GL11.GL_STENCIL_TEST);
        RenderSystem.disableDepthTest();
        GL11.glDisable(GL11.GL_DEPTH_TEST);
        GL13.glActiveTexture(GL13.GL_TEXTURE0);
        RenderSystem.activeTexture(GL13.GL_TEXTURE0);
        RenderSystem.disableCull();
        // 还原着色器颜色为白色：Skia 不碰这个 uniform，但打开 Screen 时 MC 可能留成非白，
        // 而 ClickGui 全走 Skia、不会像 HUD 路径那样被 MC 每帧重置，导致下一帧世界渲染偏色。
        RenderSystem.setShaderColor(1.0F, 1.0F, 1.0F, 1.0F);
    }

}
