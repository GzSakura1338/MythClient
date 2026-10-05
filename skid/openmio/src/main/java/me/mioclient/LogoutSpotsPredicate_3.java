package me.mioclient;

import me.mioclient.module.render.LogoutSpots;

/* loaded from: mio-yarn.jar:me/mioclient/LogoutSpotsPredicate_3.class */
public class LogoutSpotsPredicate_3 implements java.util.function.Predicate {
    public LogoutSpots logoutSpots;

    public LogoutSpotsPredicate_3(LogoutSpots logoutSpots) {
        this.logoutSpots = logoutSpots;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.logoutSpots.model.getValue() == LogoutSpots.LogoutSpotsMode_2.SIMPLE || this.logoutSpots.model.getValue() == LogoutSpots.LogoutSpotsMode_2.BOTH;
    }
}
