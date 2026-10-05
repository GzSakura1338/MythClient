package me.mioclient;

import me.mioclient.module.render.Shader;

/* loaded from: mio-yarn.jar:me/mioclient/ShaderPredicate_7.class */
public class ShaderPredicate_7 implements java.util.function.Predicate {
    public Shader shader;

    public ShaderPredicate_7(Shader shader) {
        this.shader = shader;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.shader.lineWidth.getValue().intValue() > 2;
    }
}
