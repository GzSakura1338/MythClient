package today.starain.module.impl.render;

import com.mojang.blaze3d.platform.GlStateManager;
import com.mojang.blaze3d.systems.RenderSystem;
import net.minecraft.client.Minecraft;
import com.mojang.blaze3d.vertex.BufferUploader;
import today.starain.Starain;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.ui.Theme;
import today.starain.utils.render.shader.PostProcessRenderer;
import today.starain.utils.render.shader.Shader;
import today.starain.utils.render.shader.ShaderHelper;
import today.starain.value.impl.BooleanValue;
import today.starain.value.impl.ColorValue;
import today.starain.value.impl.NumberValue;

import org.lwjgl.opengl.GL30C;

import java.awt.Color;

import static org.lwjgl.opengl.GL11C.GL_DST_COLOR;
import static org.lwjgl.opengl.GL11C.GL_ONE;
import static org.lwjgl.opengl.GL11C.GL_ZERO;

public class WorldColor extends ClientModule {

    private static WorldColor cached;
    private static boolean inWorld;
    private static Shader shader;

    private final BooleanValue syncColor = new BooleanValue("Sync Color", false, this);
    private final ColorValue color = new ColorValue("Color", new Color(120, 70, 210), this, () -> !syncColor.get());
    private final NumberValue strength = new NumberValue("Strength", 0.0, 100.0, 100.0, 1.0, this);
    private final BooleanValue world = new BooleanValue("World", true, this);
    private final BooleanValue hand = new BooleanValue("Hand", false, this);

    public WorldColor() {
        setName("WorldColor");
        setCategory(Category.RENDER);
    }

    public static boolean tintHand() {
        WorldColor module = get();
        return module != null && module.isEnabled() && module.hand.get();
    }

    public static void setInWorld(boolean value) {
        inWorld = value;
    }

    public static boolean shouldTintShader(String shaderName) {
        if (!inWorld || isWorldShader(shaderName)) {
            return false;
        }
        WorldColor module = get();
        return module != null && module.isEnabled() && !module.world.get()
                && module.strength.get().floatValue() > 0.0F;
    }

    private static boolean isWorldShader(String name) {
        if (name == null) {
            return false;
        }
        return name.equals("rendertype_solid")
                || name.equals("rendertype_cutout")
                || name.equals("rendertype_cutout_mipped")
                || name.equals("rendertype_translucent")
                || name.equals("rendertype_translucent_moving_block")
                || name.equals("rendertype_tripwire")
                || name.startsWith("rendertype_sky")
                || name.startsWith("rendertype_clouds")
                || name.equals("position");
    }

    public static float[] modulate(float[] original) {
        WorldColor module = get();
        if (module == null || original == null || original.length < 4) {
            return original;
        }
        float s = module.strength.get().floatValue() / 100.0F;
        Color base = module.syncColor.get() ? Theme.accent(0) : module.color.get();
        float tr = 1.0F - s + (base.getRed() / 255.0F) * s;
        float tg = 1.0F - s + (base.getGreen() / 255.0F) * s;
        float tb = 1.0F - s + (base.getBlue() / 255.0F) * s;
        return new float[]{original[0] * tr, original[1] * tg, original[2] * tb, original[3]};
    }

    public static void renderOverlay() {
        WorldColor module = get();
        if (module == null || !module.isEnabled() || !module.world.get()) {
            return;
        }
        float s = module.strength.get().floatValue() / 100.0F;
        if (s <= 0.0F) {
            return;
        }

        if (shader == null) {
            shader = new Shader("worldcolor.vert", "worldcolor.frag");
        }

        Color base = module.syncColor.get() ? Theme.accent(0) : module.color.get();
        float r = 1.0F - s + (base.getRed() / 255.0F) * s;
        float g = 1.0F - s + (base.getGreen() / 255.0F) * s;
        float b = 1.0F - s + (base.getBlue() / 255.0F) * s;

        int previousFbo = GlStateManager._getInteger(GL30C.GL_FRAMEBUFFER_BINDING);
        Minecraft.getInstance().getMainRenderTarget().bindWrite(true);

        RenderSystem.enableBlend();
        GlStateManager._blendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_ONE, GL_ONE);
        RenderSystem.disableDepthTest();
        RenderSystem.depthMask(false);
        RenderSystem.disableCull();

        shader.bind();
        shader.set("uColor", r, g, b, 1.0);
        PostProcessRenderer.beginRender();
        PostProcessRenderer.render();
        PostProcessRenderer.endRender();

        BufferUploader.reset();
        ShaderHelper.useProgram(0);

        if (previousFbo != 0) {
            GlStateManager._glBindFramebuffer(GL30C.GL_FRAMEBUFFER, previousFbo);
        }

        RenderSystem.defaultBlendFunc();
        RenderSystem.disableBlend();
        RenderSystem.enableDepthTest();
        RenderSystem.depthMask(true);
        RenderSystem.enableCull();
        RenderSystem.setShaderColor(1.0F, 1.0F, 1.0F, 1.0F);
    }

    private static WorldColor get() {
        if (cached != null) {
            return cached;
        }
        Starain starain = Starain.getInstance();
        if (starain == null || starain.getModuleManager() == null) {
            return null;
        }
        cached = starain.getModuleManager().getModule(WorldColor.class);
        return cached;
    }
}
