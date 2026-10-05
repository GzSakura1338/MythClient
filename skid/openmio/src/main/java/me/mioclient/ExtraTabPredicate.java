package me.mioclient;

import me.mioclient.module.misc.ExtraTab;

/* loaded from: mio-yarn.jar:me/mioclient/ExtraTabPredicate.class */
public class ExtraTabPredicate implements java.util.function.Predicate {
    public ExtraTab extraTab;

    public ExtraTabPredicate(ExtraTab extraTab) {
        this.extraTab = extraTab;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.extraTab.highlight.is623();
    }
}
