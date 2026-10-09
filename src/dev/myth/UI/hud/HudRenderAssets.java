package dev.myth.UI.hud;

import com.mojang.blaze3d.opengl.GlTexture;
import com.mojang.blaze3d.systems.RenderSystem;
import imgui.ImDrawList;
import java.awt.image.BufferedImage;
import java.io.IOException;
import java.util.HashMap;
import javax.imageio.ImageIO;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.render.GuiItemAtlas;
import net.minecraft.client.gui.render.pip.GuiEntityRenderer;
import net.minecraft.client.renderer.item.TrackingItemStackRenderState;
import net.minecraft.client.renderer.state.gui.BlitRenderState;
import net.minecraft.client.renderer.state.gui.GuiRenderState;
import net.minecraft.client.renderer.state.gui.pip.GuiEntityRenderState;
import net.minecraft.world.item.ItemDisplayContext;
import net.minecraft.world.item.ItemStack;
import org.joml.Quaternionf;
import org.joml.Vector3f;
import org.lwjgl.system.MemoryUtil;
import static org.lwjgl.opengl.GL33C.*;

/** Native 26.2 item/entity textures subsequently composed by ImGui. */
public final class HudRenderAssets implements AutoCloseable {
    private static final int ATLAS_SIZE = 1024, SLOT_SIZE = 64;
    private GuiItemAtlas items;
    private GuiEntityRenderer playerRenderer;
    private final java.util.Map<String, Integer> textures = new HashMap<>();
    /** The model identities this frame asked for, which is what the atlas is allowed to keep. */
    private final java.util.Set<Object> frameItems = new java.util.HashSet<>();
    void beginFrame() {
        if (items != null) items.endFrame();
        frameItems.clear();
    }
    public void item(ImDrawList draw, ItemStack stack, float x, float y, float scale, float opacity) {
        if (stack.isEmpty() || opacity <= 0) return;
        var client = Minecraft.getInstance();
        TrackingItemStackRenderState state = new TrackingItemStackRenderState();
        client.getItemModelResolver().updateForTopItem(state, stack, ItemDisplayContext.GUI, client.level, client.player, 0);
        if (state.isEmpty()) { skipped("no model layers", stack); return; }
        GuiItemAtlas.SlotView slot;
        try (var ignored = new NativeState()) {
            if (items == null) items = new GuiItemAtlas(client.gameRenderer.featureRenderDispatcher(), ATLAS_SIZE, SLOT_SIZE);
            // GuiRenderer.prepareItemAtlas reclaims the slots the frame does not need before it
            // draws anything. Without that the atlas only ever fills - the allocator keys on the
            // model identity, and an inventory churns through far more than 256 of them in a
            // session - and then every further item silently gets no slot. Hand it this frame's
            // identities including the one being asked for, so hasSpaceForAll fails and
            // reclaimSpaceFor actually frees a slot that nothing on screen is using.
            frameItems.add(state.getModelIdentity());
            slot = items.getOrUpdate(state);
            if (slot == null && items.tryPrepareFor(frameItems)) slot = items.getOrUpdate(state);
        }
        // Recreating the atlas here would close the texture that this frame's queued ImGui draw
        // commands already point at, blanking every item drawn before it. Skip one frame instead.
        if (slot == null) { skipped("no atlas slot", stack); return; }
        if (!(slot.textureView().texture() instanceof GlTexture texture)) { skipped("atlas is not a GL texture", stack); return; }
        draw.addImage(texture.glId(), x, y, x + 16 * scale, y + 16 * scale,
                slot.u0(), slot.v0(), slot.u1(), slot.v1(), HudDraw.packed(0xffffffff, opacity));
    }

    /**
     * Diagnostic for items that draw their count and durability bar but no icon: every way out of
     * item() above is silent, and UiManager swallows render exceptions, so nothing reaches the log
     * on its own. One line per reason per item, then it stays quiet.
     */
    private final java.util.Set<String> skips = new java.util.HashSet<>();
    private void skipped(String reason, ItemStack stack) {
        String key = reason + " " + stack.getItem();
        if (skips.size() < 64 && skips.add(key))
            dev.myth.Myth.LOGGER.warn("Myth HUD drew no icon for {}: {} (atlas slots used: {})",
                    stack.getItem(), reason, frameItems.size());
    }
    public void decoratedItem(HudElement.RenderContext context, ItemStack stack, float x, float y, float scale, float opacity) {
        item(context.draw(), stack, x, y, scale, opacity);
        if (stack.isEmpty()) return;
        if (stack.getCount() > 1) {
            String count = Integer.toString(stack.getCount());
            context.fonts().draw(context.draw(), count, x + (17 - context.fonts().width(count)) * scale, y + 9 * scale, 0xffffffff, opacity, scale);
        }
        if (stack.isBarVisible()) {
            HudDraw.rect(context.draw(), x + 2 * scale, y + 13 * scale, 13 * scale, 2 * scale, 0xff000000, opacity);
            HudDraw.rect(context.draw(), x + 2 * scale, y + 13 * scale, stack.getBarWidth() * scale, scale, 0xff000000 | stack.getBarColor(), opacity);
        }
        if (Minecraft.getInstance().player != null) {
            float cooldown = Minecraft.getInstance().player.getCooldowns().getCooldownPercent(stack, 0);
            if (cooldown > 0) HudDraw.rect(context.draw(), x, y + (1 - cooldown) * 16 * scale, 16 * scale, cooldown * 16 * scale, 0x7fffffff, opacity);
        }
    }
    public void player(ImDrawList draw, float x, float y, float scale, float opacity) {
        entity(draw, Minecraft.getInstance().player, x, y, 50, 80, 35, scale, opacity);
    }

    /** Same picture-in-picture path as the local player, for any living entity and panel size. */
    public void entity(ImDrawList draw, net.minecraft.world.entity.LivingEntity target, float x, float y,
                       int boxWidth, int boxHeight, float modelScale, float scale, float opacity) {
        var client = Minecraft.getInstance(); if (target == null || opacity <= 0) return;
        var entityState = client.getEntityRenderDispatcher().getRenderer(target).createRenderState(target, 1);
        entityState.shadowPieces.clear(); entityState.outlineColor = 0; entityState.lightCoords = 15728880;
        GuiRenderState output = new GuiRenderState();
        try (var ignored = new NativeState()) {
            if (playerRenderer == null) playerRenderer = new GuiEntityRenderer(client.getEntityRenderDispatcher());
            playerRenderer.prepare(new GuiEntityRenderState(entityState, new Vector3f(0, target.getBbHeight() / 2, 0),
                    new Quaternionf().rotateZ((float) Math.PI), new Quaternionf(), 0, 0, boxWidth, boxHeight, modelScale, null),
                    output, client.gameRenderer.featureRenderDispatcher(), 2);
        }
        output.forEachElement(element -> {
            if (element instanceof BlitRenderState blit && blit.textureSetup().texure0().texture() instanceof GlTexture texture)
                draw.addImage(texture.glId(), x, y, x + boxWidth * scale, y + boxHeight * scale, 0, 1, 1, 0,
                        HudDraw.packed(0xffffffff, opacity));
        }, GuiRenderState.TraverseRange.ALL);
    }
    public int texture(String path) { return textures.computeIfAbsent(path, HudRenderAssets::load); }
    private static int load(String path) {
        try (var stream = HudRenderAssets.class.getResourceAsStream(path)) {
            if (stream == null) throw new IOException("Missing HUD resource " + path);
            BufferedImage image = ImageIO.read(stream);
            int previous = glGetInteger(GL_TEXTURE_BINDING_2D), pbo = glGetInteger(GL_PIXEL_UNPACK_BUFFER_BINDING);
            int alignment = glGetInteger(GL_UNPACK_ALIGNMENT), rowLength = glGetInteger(GL_UNPACK_ROW_LENGTH);
            int skipRows = glGetInteger(GL_UNPACK_SKIP_ROWS), skipPixels = glGetInteger(GL_UNPACK_SKIP_PIXELS);
            var pixels = MemoryUtil.memAlloc(image.getWidth() * image.getHeight() * 4);
            try {
                for (int y = 0; y < image.getHeight(); y++) for (int x = 0; x < image.getWidth(); x++) {
                    int argb = image.getRGB(x, y); pixels.put((byte) (argb >> 16)).put((byte) (argb >> 8)).put((byte) argb).put((byte) (argb >>> 24));
                }
                pixels.flip(); int id = glGenTextures(); glBindTexture(GL_TEXTURE_2D, id); glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1); glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                glPixelStorei(GL_UNPACK_SKIP_ROWS, 0); glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.getWidth(), image.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                return id;
            } finally {
                MemoryUtil.memFree(pixels); glBindTexture(GL_TEXTURE_2D, previous); glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
                glPixelStorei(GL_UNPACK_ALIGNMENT, alignment); glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);
                glPixelStorei(GL_UNPACK_SKIP_ROWS, skipRows); glPixelStorei(GL_UNPACK_SKIP_PIXELS, skipPixels);
            }
        } catch (IOException error) { throw new IllegalStateException(error); }
    }
    private static final class NativeState implements AutoCloseable {
        final com.mojang.blaze3d.buffers.GpuBufferSlice projection = RenderSystem.getProjectionMatrixBuffer();
        final com.mojang.blaze3d.ProjectionType projectionType = RenderSystem.getProjectionType();
        final com.mojang.blaze3d.textures.GpuTextureView color = RenderSystem.outputColorTextureOverride, depth = RenderSystem.outputDepthTextureOverride;
        final int read = glGetInteger(GL_READ_FRAMEBUFFER_BINDING), write = glGetInteger(GL_DRAW_FRAMEBUFFER_BINDING);
        final int[] viewport = new int[4], scissorBox = new int[4];
        final boolean scissor = glIsEnabled(GL_SCISSOR_TEST), srgb = glIsEnabled(GL_FRAMEBUFFER_SRGB);
        NativeState() { glGetIntegerv(GL_VIEWPORT, viewport); glGetIntegerv(GL_SCISSOR_BOX, scissorBox); }
        @Override public void close() {
            RenderSystem.outputColorTextureOverride = color; RenderSystem.outputDepthTextureOverride = depth;
            RenderSystem.setProjectionMatrix(projection, projectionType);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, read); glBindFramebuffer(GL_DRAW_FRAMEBUFFER, write);
            glViewport(viewport[0], viewport[1], viewport[2], viewport[3]); glScissor(scissorBox[0], scissorBox[1], scissorBox[2], scissorBox[3]);
            if (scissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
            if (srgb) glEnable(GL_FRAMEBUFFER_SRGB); else glDisable(GL_FRAMEBUFFER_SRGB);
        }
    }
    @Override public void close() {
        if (items != null) { items.close(); items = null; }
        if (playerRenderer != null) { playerRenderer.close(); playerRenderer = null; }
        textures.values().forEach(org.lwjgl.opengl.GL33C::glDeleteTextures); textures.clear();
    }
}
