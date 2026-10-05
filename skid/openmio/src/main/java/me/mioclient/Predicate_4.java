package me.mioclient;

import me.mioclient.module.client.UI;

/* loaded from: mio-yarn.jar:me/mioclient/Predicate_4.class */
public class Predicate_4 implements java.util.function.Predicate {
    public UI uI;

    public Predicate_4(UI ui) {
        this.uI = ui;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.uI.elements.is623();
    }
}
