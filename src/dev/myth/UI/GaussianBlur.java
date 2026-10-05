package dev.myth.UI;

import imgui.ImDrawList;
import imgui.ImGui;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import static org.lwjgl.opengl.GL33C.*;

/** Two separable Gaussian passes over the game backdrop, cached at its own FPS. */
public final class GaussianBlur implements AutoCloseable {
    private final int[] framebuffers = new int[3], textures = new int[3];
    private int program, vao, sourceUniform, stepUniform, radiusUniform, weightsUniform;
    private int width, height, sourceWidth, sourceHeight, cachedFps;
    private float cachedStrength;
    private long lastUpdate;
    private boolean ready;

    public void invalidate() { ready = false; lastUpdate = 0; }

    public void prepare(int screenWidth, int screenHeight, float strength, int fps) {
        if (screenWidth <= 0 || screenHeight <= 0) { invalidate(); return; }
        long now = System.nanoTime();
        if (ready && screenWidth == sourceWidth && screenHeight == sourceHeight
                && strength == cachedStrength && fps == cachedFps && now - lastUpdate < 1_000_000_000L / fps) return;
        // Reduce large kernels as well as the framebuffer. Sigma remains in
        // physical screen pixels, independently of the working resolution.
        int reduction = Math.max(2, (int) Math.ceil(3 * strength / 64));
        int nextWidth = Math.max(1, (screenWidth + reduction - 1) / reduction);
        int nextHeight = Math.max(1, (screenHeight + reduction - 1) / reduction);
        GlState state = new GlState();
        try {
            if (program == 0) initialize();
            if (width != nextWidth || height != nextHeight) resize(nextWidth, nextHeight);
            glDisable(GL_SCISSOR_TEST); glDisable(GL_BLEND); glDisable(GL_DEPTH_TEST);
            glDisable(GL_STENCIL_TEST); glDisable(GL_CULL_FACE); glDisable(GL_FRAMEBUFFER_SRGB);
            glDisable(GL_RASTERIZER_DISCARD); glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glColorMask(true, true, true, true);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            int readBuffer = glGetInteger(GL_READ_BUFFER);
            try {
                glReadBuffer(GL_BACK); glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffers[0]);
                glBlitFramebuffer(0, 0, screenWidth, screenHeight, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
            } finally { glReadBuffer(readBuffer); }
            glViewport(0, 0, width, height);
            glUseProgram(program); glBindVertexArray(vao); glBindSampler(0, 0);
            glUniform1i(sourceUniform, 0);
            pass(textures[0], framebuffers[1], strength * width / screenWidth, 1f / width, 0);
            pass(textures[1], framebuffers[2], strength * height / screenHeight, 0, 1f / height);
            sourceWidth = screenWidth; sourceHeight = screenHeight;
            cachedStrength = strength; cachedFps = fps; lastUpdate = now; ready = true;
        } finally { state.restore(); }
    }

    public void draw(ImDrawList draw, float x, float y, float regionWidth, float regionHeight, float opacity) {
        if (!ready || regionWidth <= 0 || regionHeight <= 0 || opacity <= 0) return;
        float screenWidth = ImGui.getIO().getDisplaySizeX(), screenHeight = ImGui.getIO().getDisplaySizeY();
        int color = Math.clamp(Math.round(opacity * 255), 0, 255) << 24 | 0xffffff;
        // OpenGL's origin is bottom-left; ImGui's is top-left. Crop the cached
        // backdrop at the panel's current position rather than blurring UI text.
        float left = Math.max(0, x), top = Math.max(0, y);
        float right = Math.min(screenWidth, x + regionWidth), bottom = Math.min(screenHeight, y + regionHeight);
        if (right <= left || bottom <= top) return;
        draw.addImage(textures[2], left, top, right, bottom, left / screenWidth, 1 - top / screenHeight,
                right / screenWidth, 1 - bottom / screenHeight, color);
    }

    private void pass(int texture, int target, float sigma, float stepX, float stepY) {
        sigma = Math.max(.01f, sigma);
        int radius = Math.min(64, (int) Math.ceil(3 * sigma));
        float[] weights = new float[radius + 1];
        float total = 0;
        for (int i = 0; i <= radius; i++) {
            weights[i] = (float) Math.exp(-i * i / (2 * sigma * sigma));
            total += weights[i] * (i == 0 ? 1 : 2);
        }
        for (int i = 0; i <= radius; i++) weights[i] /= total;
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target); glBindTexture(GL_TEXTURE_2D, texture);
        glUniform2f(stepUniform, stepX, stepY); glUniform1i(radiusUniform, radius);
        glUniform1fv(weightsUniform, weights); glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    private void initialize() {
        int vertex = shader(GL_VERTEX_SHADER, "gaussian.vsh"), fragment = 0, nextProgram = 0;
        try {
            fragment = shader(GL_FRAGMENT_SHADER, "gaussian.fsh"); nextProgram = glCreateProgram();
            glAttachShader(nextProgram, vertex); glAttachShader(nextProgram, fragment); glLinkProgram(nextProgram);
            if (glGetProgrami(nextProgram, GL_LINK_STATUS) == GL_FALSE)
                throw new IllegalStateException("Could not link Gaussian blur: " + glGetProgramInfoLog(nextProgram));
            program = nextProgram; nextProgram = 0; vao = glGenVertexArrays();
            sourceUniform = glGetUniformLocation(program, "sourceTexture");
            stepUniform = glGetUniformLocation(program, "texelStep");
            radiusUniform = glGetUniformLocation(program, "radius");
            weightsUniform = glGetUniformLocation(program, "weights[0]");
        } finally {
            glDeleteShader(vertex); if (fragment != 0) glDeleteShader(fragment);
            if (nextProgram != 0) glDeleteProgram(nextProgram);
        }
    }

    private static int shader(int type, String name) {
        String source;
        try (var stream = GaussianBlur.class.getResourceAsStream("/assets/myth/shaders/" + name)) {
            if (stream == null) throw new IOException("Missing shader: " + name);
            source = new String(stream.readAllBytes(), StandardCharsets.UTF_8);
        } catch (IOException exception) { throw new IllegalStateException("Could not load Gaussian blur shader", exception); }
        int shader = glCreateShader(type); glShaderSource(shader, source); glCompileShader(shader);
        if (glGetShaderi(shader, GL_COMPILE_STATUS) == GL_FALSE) {
            String error = glGetShaderInfoLog(shader); glDeleteShader(shader);
            throw new IllegalStateException("Could not compile Gaussian blur: " + error);
        }
        return shader;
    }

    private void resize(int nextWidth, int nextHeight) {
        destroyTargets(); width = nextWidth; height = nextHeight;
        for (int i = 0; i < textures.length; i++) {
            textures[i] = glGenTextures(); glBindTexture(GL_TEXTURE_2D, textures[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0L);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            framebuffers[i] = glGenFramebuffers(); glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffers[i]);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[i], 0);
            if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                throw new IllegalStateException("Could not create Gaussian blur framebuffer");
        }
    }

    private void destroyTargets() {
        for (int i = 0; i < textures.length; i++) {
            if (framebuffers[i] != 0) glDeleteFramebuffers(framebuffers[i]);
            if (textures[i] != 0) glDeleteTextures(textures[i]);
            framebuffers[i] = textures[i] = 0;
        }
        width = height = 0; invalidate();
    }

    @Override public void close() {
        destroyTargets(); if (program != 0) glDeleteProgram(program); if (vao != 0) glDeleteVertexArrays(vao);
        program = vao = 0;
    }

    private static final class GlState {
        final int read = glGetInteger(GL_READ_FRAMEBUFFER_BINDING), write = glGetInteger(GL_DRAW_FRAMEBUFFER_BINDING);
        final int program = glGetInteger(GL_CURRENT_PROGRAM), vao = glGetInteger(GL_VERTEX_ARRAY_BINDING);
        final int activeTexture = glGetInteger(GL_ACTIVE_TEXTURE), unpackBuffer = glGetInteger(GL_PIXEL_UNPACK_BUFFER_BINDING);
        final int[] viewport = new int[4], polygonMode = new int[2];
        final int[] colorMask = new int[4];
        final int[] flags = {GL_SCISSOR_TEST, GL_BLEND, GL_DEPTH_TEST, GL_STENCIL_TEST, GL_CULL_FACE, GL_FRAMEBUFFER_SRGB, GL_RASTERIZER_DISCARD};
        final boolean[] enabled = new boolean[flags.length];
        final int texture, sampler;
        GlState() {
            glGetIntegerv(GL_VIEWPORT, viewport); glGetIntegerv(GL_POLYGON_MODE, polygonMode); glGetIntegerv(GL_COLOR_WRITEMASK, colorMask);
            for (int i = 0; i < flags.length; i++) enabled[i] = glIsEnabled(flags[i]);
            glActiveTexture(GL_TEXTURE0); texture = glGetInteger(GL_TEXTURE_BINDING_2D); sampler = glGetInteger(GL_SAMPLER_BINDING);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        }
        void restore() {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, read); glBindFramebuffer(GL_DRAW_FRAMEBUFFER, write);
            glUseProgram(program); glBindVertexArray(vao); glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
            glPolygonMode(GL_FRONT_AND_BACK, polygonMode[0]); glColorMask(colorMask[0] != 0, colorMask[1] != 0, colorMask[2] != 0, colorMask[3] != 0);
            for (int i = 0; i < flags.length; i++) { if (enabled[i]) glEnable(flags[i]); else glDisable(flags[i]); }
            glBindTexture(GL_TEXTURE_2D, texture); glBindSampler(0, sampler); glActiveTexture(activeTexture);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpackBuffer);
        }
    }
}
