package me.mioclient;

import me.mioclient.module.movement.FastWeb;

/* loaded from: mio-yarn.jar:me/mioclient/FastWebPredicate.class */
public class FastWebPredicate implements java.util.function.Predicate {
    public FastWeb fastWeb;

    public FastWebPredicate(FastWeb fastWeb) {
        this.fastWeb = fastWeb;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.fastWeb.mode.getValue() != FastWeb.FastWebPredicateMode.GRIM;
    }
}
