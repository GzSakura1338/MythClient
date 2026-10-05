package dev.myth.UI;

import dev.myth.Myth;
import imgui.ImGui;
import imgui.gl3.ImGuiImplGl3;
import imgui.glfw.ImGuiImplGlfw;
import org.lwjgl.glfw.GLFW;
import static org.lwjgl.opengl.GL33C.*;

/** One context; callbacks are forwarded explicitly so Minecraft owns its GLFW callbacks. */
public final class ImGuiRenderer implements AutoCloseable {
    private final ImGuiImplGlfw glfw = new ImGuiImplGlfw() {
        // Submit only logical coordinates below; the fallback queues unscaled ones.
        @Override protected void updateMouseData() {}
    };
    private final ImGuiImplGl3 gl3 = new ImGuiImplGl3();
    private final GaussianBlur blur = new GaussianBlur();
    private boolean blurEnabled;
    private float blurStrength;
    private int blurFps;
    private final boolean[] clicked = new boolean[5];
    private final double[] cursorX = new double[1], cursorY = new double[1];
    private boolean initialized;
    private boolean contextCreated;
    private boolean glfwInitialized;
    private boolean gl3Initialized;
    private boolean focused;
    private long window;
    private int msaaFramebuffer;
    private int msaaColor;
    private int msaaWidth;
    private int msaaHeight;
    private int msaaSamples;

    public boolean initialize(long window) {
        if (initialized) return true;
        this.window = window;
        if (GLFW.glfwGetCurrentContext() != window) return false;
        ImGui.createContext();
        contextCreated = true;
        try {
            ImGui.getIO().setIniFilename(null);
            ImGui.getIO().setLogFilename(null);
            ImGui.getIO().setConfigInputTrickleEventQueue(false);
            ImGui.getIO().getFonts().addFontDefaultBitmap();
            if (!(glfwInitialized = glfw.initForOpenGL(window, false))) throw new IllegalStateException("Could not initialize ImGui GLFW backend");
            if (!(gl3Initialized = gl3.init("#version 330 core"))) throw new IllegalStateException("Could not initialize ImGui OpenGL backend");
            initialized = true;
            focused = GLFW.glfwGetWindowAttrib(window, GLFW.GLFW_FOCUSED) != 0;
            Myth.LOGGER.info("Mio ClickGUI ImGui renderer initialized");
            return true;
        } catch (RuntimeException | LinkageError exception) { close(); throw exception; }
    }

    public void configureBlur(boolean enabled, float strength, int fps) {
        if (enabled != blurEnabled) blur.invalidate();
        blurEnabled = enabled; blurStrength = strength; blurFps = fps;
    }
    public void invalidateBlur() { blur.invalidate(); }
    public void drawBlur(imgui.ImDrawList draw, float x, float y, float width, float height, float opacity) {
        if (blurEnabled) blur.draw(draw, x, y, width, height, opacity);
    }

    /** Edge-triggered clicks, latched once so both passes observe the same frame. */
    public boolean isMouseClicked(int button) { return button >= 0 && button < clicked.length && clicked[button]; }

    public void render(Runnable below, Runnable above, float logicalScale, int samples) {
        if (!initialized) return;
        boolean nextFocus = GLFW.glfwGetWindowAttrib(window, GLFW.GLFW_FOCUSED) != 0;
        if (focused != nextFocus) { glfw.windowFocusCallback(window, nextFocus); focused = nextFocus; }
        gl3.newFrame(); glfw.newFrame();
        var io = ImGui.getIO();
        float displayX = io.getDisplaySizeX(), displayY = io.getDisplaySizeY();
        float framebufferX = io.getDisplayFramebufferScaleX(), framebufferY = io.getDisplayFramebufferScaleY();
        int width = Math.round(displayX * framebufferX), height = Math.round(displayY * framebufferY);
        io.setDisplaySize(displayX / logicalScale, displayY / logicalScale);
        io.setDisplayFramebufferScale(framebufferX * logicalScale, framebufferY * logicalScale);
        // IO.MousePos is the previous frame's processed (already scaled) position.
        // Read the current GLFW position instead of repeatedly scaling that value.
        GLFW.glfwGetCursorPos(window, cursorX, cursorY);
        io.addMousePosEvent(focused ? (float) cursorX[0] / logicalScale : -Float.MAX_VALUE,
                focused ? (float) cursorY[0] / logicalScale : -Float.MAX_VALUE);
        if (!blurEnabled) { pass(() -> { below.run(); above.run(); }, width, height, samples, true); return; }
        // Mio samples the composited backdrop. Flush everything below the blur first, so a
        // blurred panel frosts the HUD elements under it instead of painting over them.
        pass(below, width, height, samples, true);
        blur.prepare(width, height, blurStrength * logicalScale * framebufferX, blurFps);
        pass(above, width, height, samples, false);
    }

    private void pass(Runnable content, int width, int height, int samples, boolean latchInput) {
        ImGui.newFrame();
        if (latchInput) for (int button = 0; button < clicked.length; button++) clicked[button] = ImGui.isMouseClicked(button);
        content.run();
        ImGui.render();

        int read = glGetInteger(GL_READ_FRAMEBUFFER_BINDING), write = glGetInteger(GL_DRAW_FRAMEBUFFER_BINDING);
        boolean scissor = glIsEnabled(GL_SCISSOR_TEST);
        boolean srgb = glIsEnabled(GL_FRAMEBUFFER_SRGB);
        boolean multisample = glIsEnabled(GL_MULTISAMPLE);
        try {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDisable(GL_FRAMEBUFFER_SRGB);
            if (samples > 1) {
                glEnable(GL_MULTISAMPLE);
                ensureMsaa(width, height, samples);
                glDisable(GL_SCISSOR_TEST);
                glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, msaaFramebuffer);
                glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
            }
            gl3.renderDrawData(ImGui.getDrawData());
            if (samples > 1) {
                glDisable(GL_SCISSOR_TEST);
                glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFramebuffer);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
            }
        } finally {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, read); glBindFramebuffer(GL_DRAW_FRAMEBUFFER, write);
            if (scissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
            if (srgb) glEnable(GL_FRAMEBUFFER_SRGB); else glDisable(GL_FRAMEBUFFER_SRGB);
            if (multisample) glEnable(GL_MULTISAMPLE); else glDisable(GL_MULTISAMPLE);
        }
    }

    private void ensureMsaa(int width, int height, int samples) {
        samples = Math.min(samples, glGetInteger(GL_MAX_SAMPLES));
        if (msaaFramebuffer != 0 && width == msaaWidth && height == msaaHeight && samples == msaaSamples) return;
        destroyMsaa();
        int previous = glGetInteger(GL_RENDERBUFFER_BINDING);
        msaaFramebuffer = glGenFramebuffers(); msaaColor = glGenRenderbuffers();
        glBindRenderbuffer(GL_RENDERBUFFER, msaaColor);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, width, height);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, msaaFramebuffer);
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaaColor);
        glBindRenderbuffer(GL_RENDERBUFFER, previous);
        if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw new IllegalStateException("Could not create ClickGUI MSAA framebuffer");
        msaaWidth = width; msaaHeight = height; msaaSamples = samples;
    }

    public void key(int key, int scan, int action, int mods) {
        if (initialized && key != GLFW.GLFW_KEY_UNKNOWN) glfw.keyCallback(window, key, scan, action, mods);
    }
    public void character(int codepoint) { if (initialized) glfw.charCallback(window, codepoint); }
    public void button(int button, int action, int mods) { if (initialized) glfw.mouseButtonCallback(window, button, action, mods); }
    public void scroll(double x, double y) { if (initialized) glfw.scrollCallback(window, x, y); }
    public void clearInput() {
        if (!initialized) return;
        java.util.Arrays.fill(clicked, false);
        ImGui.getIO().clearInputKeys(); ImGui.getIO().clearInputMouse();
        ImGui.getIO().clearEventsQueue();
        GLFW.glfwSetCursor(window, 0);
    }
    private void destroyMsaa() {
        if (msaaFramebuffer != 0) glDeleteFramebuffers(msaaFramebuffer);
        if (msaaColor != 0) glDeleteRenderbuffers(msaaColor);
        msaaFramebuffer = msaaColor = 0;
    }
    @Override public void close() {
        if (!contextCreated) return;
        destroyMsaa(); blur.close();
        try { if (gl3Initialized) gl3.shutdown(); }
        finally {
            try { if (glfwInitialized) glfw.shutdown(); }
            finally { ImGui.destroyContext(); initialized = contextCreated = glfwInitialized = gl3Initialized = false; }
        }
    }
}
