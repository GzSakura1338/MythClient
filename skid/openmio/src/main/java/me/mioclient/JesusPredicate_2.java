package me.mioclient;

import me.mioclient.module.movement.Jesus;

/* loaded from: mio-yarn.jar:me/mioclient/JesusPredicate_2.class */
public class JesusPredicate_2 implements java.util.function.Predicate {
    public Jesus jesus;

    public JesusPredicate_2(Jesus jesus) {
        this.jesus = jesus;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.jesus.mode.getValue() == Jesus.JesusMode.SOLID;
    }
}
