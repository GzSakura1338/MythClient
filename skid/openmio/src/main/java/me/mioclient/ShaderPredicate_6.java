package me.mioclient;

import me.mioclient.module.render.Shader;

/* loaded from: mio-yarn.jar:me/mioclient/ShaderPredicate_6.class */
public class ShaderPredicate_6 implements java.util.function.Predicate {
    public Shader shader;

    public ShaderPredicate_6(Shader shader) {
        this.shader = shader;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.shader.shader.getValue() == Shader.MixinHeldItemRendererMode.GRADIENT;
    }
}
