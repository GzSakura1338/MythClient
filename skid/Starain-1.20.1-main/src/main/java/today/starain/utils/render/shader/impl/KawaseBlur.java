package today.starain.utils.render.shader.impl;

import net.minecraft.client.Minecraft;
import today.starain.module.impl.client.HUD;
import today.starain.utils.render.shader.Framebuffer;
import today.starain.utils.render.shader.PostProcessRenderer;
import today.starain.utils.render.shader.Shader;
import today.starain.utils.render.shader.ShaderHelper;
import today.starain.utils.timer.TimerUtils;

public class KawaseBlur {

    public static final KawaseBlur INSTANCE = new KawaseBlur();

    private static Shader shaderDown, shaderUp, shaderPassthrough;
    private final Framebuffer[] fbos = new Framebuffer[6];
    private final TimerUtils timer = new TimerUtils();
    private boolean firstTick = true;

    public void resize() {
        for (int i = 0; i < fbos.length; i++) {
            if (fbos[i] != null) {
                fbos[i].resize();
            } else {
                fbos[i] = new Framebuffer(1 / Math.pow(2, i));
            }
        }
    }

    public void draw(float radius, int iterations) {

        if (shaderDown == null) {
            shaderDown = new Shader("blur.vert", "blur_down.frag");
            shaderUp = new Shader("blur.vert", "blur_up.frag");
            shaderPassthrough = new Shader("passthrough.vert", "passthrough.frag");
        }

        if (firstTick) {
            for (int i = 0; i < fbos.length; i++) {
                if (fbos[i] == null) {
                    fbos[i] = new Framebuffer(1 / Math.pow(2, i));
                }
            }
            firstTick = false;
        }


        if (HUD.isLimitBlurFpsEnabled()) {
            if (timer.delay(1000 / HUD.getBlurFps())) {
                timer.reset();
            } else {
                return;
            }
        }

        PostProcessRenderer.beginRender();

        renderToFbo(fbos[0], Minecraft.getInstance().getMainRenderTarget().getColorTextureId(), shaderDown,
                radius);

        for (int i = 0; i < iterations; i++) {
            renderToFbo(fbos[i + 1], fbos[i].texture, shaderDown, radius);
        }

        for (int i = iterations; i >= 1; i--) {
            renderToFbo(fbos[i - 1], fbos[i].texture, shaderUp, radius);
        }

        Minecraft.getInstance().getMainRenderTarget().bindWrite(true);
        shaderPassthrough.bind();
        ShaderHelper.bindTexture(fbos[0].texture);
        shaderPassthrough.set("uTexture", 0);
        PostProcessRenderer.endRender();
    }

    public int getTexture() {
        return fbos[0].texture;
    }

    private void renderToFbo(Framebuffer targetFbo, int sourceText, Shader shader, double offset) {
        targetFbo.bind();
        targetFbo.setViewport();
        shader.bind();
        ShaderHelper.bindTexture(sourceText);
        shader.set("uTexture", 0);
        shader.set("uHalfTexelSize", .5 / targetFbo.width, .5 / targetFbo.height);
        shader.set("uOffset", offset);
        PostProcessRenderer.render();
    }
}
