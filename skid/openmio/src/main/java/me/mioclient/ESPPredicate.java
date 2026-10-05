package me.mioclient;

import me.mioclient.module.render.ESP;

/* loaded from: mio-yarn.jar:me/mioclient/ESPPredicate.class */
public class ESPPredicate implements java.util.function.Predicate {
    public ESP eSP;

    public ESPPredicate(ESP esp) {
        this.eSP = esp;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.eSP.mode.getValue() == ESPPredicateMode.TEXT && this.eSP.targets.is623() && this.eSP.items.is623();
    }
}
