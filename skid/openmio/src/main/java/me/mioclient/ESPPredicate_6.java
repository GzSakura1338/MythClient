package me.mioclient;

import me.mioclient.module.render.ESP;

/* loaded from: mio-yarn.jar:me/mioclient/ESPPredicate_6.class */
public class ESPPredicate_6 implements java.util.function.Predicate {
    public ESP eSP;

    public ESPPredicate_6(ESP esp) {
        this.eSP = esp;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.eSP.mode.getValue() == ESPPredicateMode.TEXT || this.eSP.mode.getValue() == ESPPredicateMode.BOTH;
    }
}
