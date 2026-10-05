package me.mioclient;

import me.mioclient.module.render.LogoutSpots;

/* loaded from: mio-yarn.jar:me/mioclient/LogoutSpotsPredicate_9.class */
public class LogoutSpotsPredicate_9 implements java.util.function.Predicate {
    public LogoutSpots logoutSpots;

    public LogoutSpotsPredicate_9(LogoutSpots logoutSpots) {
        this.logoutSpots = logoutSpots;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.logoutSpots.model.getValue() == LogoutSpots.LogoutSpotsMode_2.SIMPLE || this.logoutSpots.model.getValue() == LogoutSpots.LogoutSpotsMode_2.BOTH;
    }
}
