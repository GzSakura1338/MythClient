package me.mioclient;

import me.mioclient.module.render.ESP;

/* loaded from: mio-yarn.jar:me/mioclient/ESPPredicate_4.class */
public class ESPPredicate_4 implements java.util.function.Predicate {
    public ESP eSP;

    public ESPPredicate_4(ESP esp) {
        this.eSP = esp;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return (this.eSP.mode.getValue() == ESPPredicateMode.BOX || this.eSP.mode.getValue() == ESPPredicateMode.BOTH) && this.eSP.targets.is623() && this.eSP.items.is623();
    }
}
