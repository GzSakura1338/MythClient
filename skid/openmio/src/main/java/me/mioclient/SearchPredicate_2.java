package me.mioclient;

import me.mioclient.module.render.Search;

/* loaded from: mio-yarn.jar:me/mioclient/SearchPredicate_2.class */
public class SearchPredicate_2 implements java.util.function.Predicate {
    public Search search;

    public SearchPredicate_2(Search search) {
        this.search = search;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.search.fill.getValue().booleanValue();
    }
}
