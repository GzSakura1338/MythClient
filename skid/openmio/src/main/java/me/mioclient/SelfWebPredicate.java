package me.mioclient;

import me.mioclient.module.combat.SelfWeb;

/* loaded from: mio-yarn.jar:me/mioclient/SelfWebPredicate.class */
public class SelfWebPredicate implements java.util.function.Predicate {
    public SelfWeb selfWeb;

    public SelfWebPredicate(SelfWeb selfWeb) {
        this.selfWeb = selfWeb;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.selfWeb.smart.is623();
    }
}
