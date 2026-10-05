package me.mioclient;

import me.mioclient.module.render.Shader;

/* loaded from: mio-yarn.jar:me/mioclient/ShaderPredicate_2.class */
public class ShaderPredicate_2 implements java.util.function.Predicate {
    public Shader shader;

    public ShaderPredicate_2(Shader shader) {
        this.shader = shader;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.shader.shader.getValue() == Shader.MixinHeldItemRendererMode.RAINBOW;
    }
}
