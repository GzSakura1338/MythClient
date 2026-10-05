package me.mioclient;

import me.mioclient.module.render.LogoutSpots;

/* loaded from: mio-yarn.jar:me/mioclient/LogoutSpotsPredicate_5.class */
public class LogoutSpotsPredicate_5 implements java.util.function.Predicate {
    public LogoutSpots logoutSpots;

    public LogoutSpotsPredicate_5(LogoutSpots logoutSpots) {
        this.logoutSpots = logoutSpots;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.logoutSpots.colors.is623();
    }
}
