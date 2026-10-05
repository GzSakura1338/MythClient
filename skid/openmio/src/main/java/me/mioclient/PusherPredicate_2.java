package me.mioclient;

import me.mioclient.module.combat.Pusher;

/* loaded from: mio-yarn.jar:me/mioclient/PusherPredicate_2.class */
public class PusherPredicate_2 implements java.util.function.Predicate {
    public Pusher pusher;

    public PusherPredicate_2(Pusher pusher) {
        this.pusher = pusher;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.pusher.render.is623();
    }
}
