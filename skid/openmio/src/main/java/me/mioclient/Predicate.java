package me.mioclient;

import me.mioclient.module.client.UI;

/* loaded from: mio-yarn.jar:me/mioclient/Predicate.class */
public class Predicate implements java.util.function.Predicate {
    public UI uI;

    public Predicate(UI ui) {
        this.uI = ui;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.uI.elements.is623() && this.uI.windowShadow.is623();
    }
}
