package me.mioclient;

import me.mioclient.module.render.Search;

/* loaded from: mio-yarn.jar:me/mioclient/SearchPredicate.class */
public class SearchPredicate implements java.util.function.Predicate {
    public Search search;

    public SearchPredicate(Search search) {
        this.search = search;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.search.tracers.getValue().booleanValue();
    }
}
