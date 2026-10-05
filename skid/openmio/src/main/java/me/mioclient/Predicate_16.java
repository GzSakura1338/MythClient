package me.mioclient;

import me.mioclient.module.client.UI;

/* loaded from: mio-yarn.jar:me/mioclient/Predicate_16.class */
public class Predicate_16 implements java.util.function.Predicate {
    public UI uI;

    public Predicate_16(UI ui) {
        this.uI = ui;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.uI.descriptions.is623() && this.uI.delay.getValue().intValue() > 0;
    }
}
