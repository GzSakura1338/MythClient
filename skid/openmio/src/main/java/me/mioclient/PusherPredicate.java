package me.mioclient;

import me.mioclient.module.combat.Pusher;

/* loaded from: mio-yarn.jar:me/mioclient/PusherPredicate.class */
public class PusherPredicate implements java.util.function.Predicate {
    public Pusher pusher;

    public PusherPredicate(Pusher pusher) {
        this.pusher = pusher;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.pusher.render.is623();
    }
}
