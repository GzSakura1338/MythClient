package dev.myth.UI;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;
import static org.lwjgl.opengl.GL33C.*;

/**
 * Mio's shader ESP pass. Vanilla already renders the chosen entities as a white silhouette into
 * LevelRenderer's entity outline target - that is what the glowing effect uses - so this only has
 * to take that texture, run one of the ported fragment shaders over it and leave the result in a
 * texture the ImGui layer can composite. Mio had to swap the world renderer's framebuffer and
 * re-render every entity itself; 26.2's frame graph makes that impossible and unnecessary.
 *
 * <p>The GL bookkeeping follows {@link GaussianBlur}: a full screen triangle from gl_VertexID with
 * no attributes, and every piece of state saved and restored around the pass.
 */
public final class EntityShader implements AutoCloseable {
    /** Uniform names match Mio's so the ported fragment shaders needed no renaming. */
    public record Style(String mode, int shapeMode, int width, boolean fastLines, float glow, int glowQuality,
                        int fill, int fillSecond, int outline, int outlineSecond,
                        int dots, int dotsRadius, float dotsAlpha,
                        float fillOffset, float fillStrength, float outlineOffset, float outlineStrength,
                        float time, float step) {}

    private final Map<String, Integer> programs = new HashMap<>();
    private int vao, framebuffer;

    /**
     * Blends the pass straight onto the game image, which is what Mio's {@code do761} does: bind
     * the main framebuffer, bind the silhouette as the source, draw one full screen quad. Going
     * through a texture and compositing later costs the straight alpha the shaders write - 26.2
     * clears the main target's alpha to 0, so a low alpha fill washes out on the way to the screen.
     *
     * @param source the GL id of the entity outline colour texture
     * @param target the GL id of the main render target's colour texture, drawn onto in place
     */
    public boolean composite(int source, int target, int width, int height, Style style) {
        if (source == 0 || target == 0 || width <= 0 || height <= 0) return false;
        GlState state = new GlState();
        try {
            int program = program(style.mode());
            if (program == 0) return false;
            if (vao == 0) vao = glGenVertexArrays();
            if (framebuffer == 0) framebuffer = glGenFramebuffers();
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target, 0);
            if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) return false;
            glDisable(GL_SCISSOR_TEST); glDisable(GL_DEPTH_TEST); glDisable(GL_STENCIL_TEST);
            glDisable(GL_CULL_FACE); glDisable(GL_FRAMEBUFFER_SRGB); glDisable(GL_RASTERIZER_DISCARD);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); glColorMask(true, true, true, true);
            // The fragment shaders discard what must not be drawn and write straight alpha for the
            // rest, so ordinary source-alpha blending is the whole of the compositing rule.
            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_ADD);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            glViewport(0, 0, width, height);
            glUseProgram(program); glBindVertexArray(vao); glBindSampler(0, 0);
            glBindTexture(GL_TEXTURE_2D, source);
            upload(program, style, width, height);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            return true;
        } finally {
            // Never leave Minecraft's colour texture attached to a framebuffer we own.
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
            state.restore();
        }
    }

    private void upload(int program, Style style, int screenWidth, int screenHeight) {
        uniform1i(program, "u_Texture", 0);
        uniform2f(program, "u_Size", screenWidth, screenHeight);
        uniform1i(program, "u_ShapeMode", style.shapeMode());
        uniform1i(program, "u_Width", style.width());
        uniform1i(program, "u_Radius", style.width());
        uniform1i(program, "u_FastLines", style.fastLines() ? 1 : 0);
        uniform1f(program, "u_GlowMultiplier", style.glow());
        uniform1i(program, "u_GlowQuality", style.glowQuality());
        uniform1i(program, "u_Dots", style.dots());
        uniform1i(program, "u_DotsRadius", Math.max(1, style.dotsRadius()));
        uniform1f(program, "u_DotsAlpha", style.dotsAlpha());
        color(program, "u_FillColor", style.fill());
        color(program, "u_FillColor2", style.fillSecond());
        color(program, "u_OutlineColor", style.outline());
        color(program, "u_OutlineColor2", style.outlineSecond());
        color(program, "u_Fill", style.fill());
        color(program, "u_Outline", style.outline());
        uniform1f(program, "u_Fill_Offset", style.fillOffset());
        uniform1f(program, "u_Fill_Strength", style.fillStrength());
        uniform1f(program, "u_Outline_Offset", style.outlineOffset());
        uniform1f(program, "u_Outline_Strength", style.outlineStrength());
        uniform1f(program, "u_Time", style.time());
        uniform1f(program, "u_Step", Math.max(.01f, style.step()));
    }

    // Uniforms absent from a given shader return -1, which glUniform ignores. That lets one Style
    // feed all four programs instead of each mode carrying its own upload path like Mio's did.
    private static void uniform1i(int program, String name, int value) {
        int location = glGetUniformLocation(program, name);
        if (location != -1) glUniform1i(location, value);
    }
    private static void uniform1f(int program, String name, float value) {
        int location = glGetUniformLocation(program, name);
        if (location != -1) glUniform1f(location, value);
    }
    private static void uniform2f(int program, String name, float x, float y) {
        int location = glGetUniformLocation(program, name);
        if (location != -1) glUniform2f(location, x, y);
    }
    private static void color(int program, String name, int argb) {
        int location = glGetUniformLocation(program, name);
        if (location == -1) return;
        glUniform4f(location, (argb >> 16 & 255) / 255f, (argb >> 8 & 255) / 255f,
                (argb & 255) / 255f, (argb >>> 24) / 255f);
    }

    private int program(String mode) {
        return programs.computeIfAbsent(mode, name -> {
            int vertex = shader(GL_VERTEX_SHADER, "esp.vsh"), fragment = 0, next = 0;
            try {
                fragment = shader(GL_FRAGMENT_SHADER, "esp_" + name.toLowerCase(java.util.Locale.ROOT) + ".fsh");
                next = glCreateProgram();
                glAttachShader(next, vertex); glAttachShader(next, fragment); glLinkProgram(next);
                if (glGetProgrami(next, GL_LINK_STATUS) == GL_FALSE)
                    throw new IllegalStateException("Could not link " + name + " ESP shader: " + glGetProgramInfoLog(next));
                int linked = next; next = 0;
                return linked;
            } finally {
                glDeleteShader(vertex);
                if (fragment != 0) glDeleteShader(fragment);
                if (next != 0) glDeleteProgram(next);
            }
        });
    }

    private static int shader(int type, String name) {
        String source;
        try (var stream = EntityShader.class.getResourceAsStream("/assets/myth/shaders/" + name)) {
            if (stream == null) throw new IOException("Missing shader: " + name);
            source = new String(stream.readAllBytes(), StandardCharsets.UTF_8);
        } catch (IOException exception) {
            throw new IllegalStateException("Could not load ESP shader " + name, exception);
        }
        int shader = glCreateShader(type);
        glShaderSource(shader, source);
        glCompileShader(shader);
        if (glGetShaderi(shader, GL_COMPILE_STATUS) == GL_FALSE) {
            String error = glGetShaderInfoLog(shader);
            glDeleteShader(shader);
            throw new IllegalStateException("Could not compile ESP shader " + name + ": " + error);
        }
        return shader;
    }

    @Override public void close() {
        if (framebuffer != 0) glDeleteFramebuffers(framebuffer);
        framebuffer = 0;
        programs.values().forEach(org.lwjgl.opengl.GL33C::glDeleteProgram);
        programs.clear();
        if (vao != 0) glDeleteVertexArrays(vao);
        vao = 0;
    }

    /**
     * GaussianBlur's save and restore set plus the blend state, because this pass runs inside
     * Minecraft's own world rendering rather than after it: everything it touches has to go back.
     */
    private static final class GlState {
        final int read = glGetInteger(GL_READ_FRAMEBUFFER_BINDING), write = glGetInteger(GL_DRAW_FRAMEBUFFER_BINDING);
        final int program = glGetInteger(GL_CURRENT_PROGRAM), vao = glGetInteger(GL_VERTEX_ARRAY_BINDING);
        final int activeTexture = glGetInteger(GL_ACTIVE_TEXTURE), unpackBuffer = glGetInteger(GL_PIXEL_UNPACK_BUFFER_BINDING);
        final int[] viewport = new int[4], polygonMode = new int[2], colorMask = new int[4];
        final int blendSrcRgb = glGetInteger(GL_BLEND_SRC_RGB), blendDstRgb = glGetInteger(GL_BLEND_DST_RGB);
        final int blendSrcAlpha = glGetInteger(GL_BLEND_SRC_ALPHA), blendDstAlpha = glGetInteger(GL_BLEND_DST_ALPHA);
        final int blendRgbEquation = glGetInteger(GL_BLEND_EQUATION_RGB), blendAlphaEquation = glGetInteger(GL_BLEND_EQUATION_ALPHA);
        final int[] flags = {GL_SCISSOR_TEST, GL_BLEND, GL_DEPTH_TEST, GL_STENCIL_TEST, GL_CULL_FACE,
                GL_FRAMEBUFFER_SRGB, GL_RASTERIZER_DISCARD};
        final boolean[] enabled = new boolean[flags.length];
        final int texture, sampler;
        GlState() {
            glGetIntegerv(GL_VIEWPORT, viewport); glGetIntegerv(GL_POLYGON_MODE, polygonMode);
            glGetIntegerv(GL_COLOR_WRITEMASK, colorMask);
            for (int i = 0; i < flags.length; i++) enabled[i] = glIsEnabled(flags[i]);
            glActiveTexture(GL_TEXTURE0);
            texture = glGetInteger(GL_TEXTURE_BINDING_2D); sampler = glGetInteger(GL_SAMPLER_BINDING);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        }
        void restore() {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, read); glBindFramebuffer(GL_DRAW_FRAMEBUFFER, write);
            glUseProgram(program); glBindVertexArray(vao);
            glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
            glPolygonMode(GL_FRONT_AND_BACK, polygonMode[0]);
            glColorMask(colorMask[0] != 0, colorMask[1] != 0, colorMask[2] != 0, colorMask[3] != 0);
            glBlendFuncSeparate(blendSrcRgb, blendDstRgb, blendSrcAlpha, blendDstAlpha);
            glBlendEquationSeparate(blendRgbEquation, blendAlphaEquation);
            for (int i = 0; i < flags.length; i++) { if (enabled[i]) glEnable(flags[i]); else glDisable(flags[i]); }
            glBindTexture(GL_TEXTURE_2D, texture); glBindSampler(0, sampler); glActiveTexture(activeTexture);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpackBuffer);
        }
    }
}
