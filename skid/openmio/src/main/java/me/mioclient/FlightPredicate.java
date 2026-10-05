package me.mioclient;

import me.mioclient.module.movement.Flight;

/* loaded from: mio-yarn.jar:me/mioclient/FlightPredicate.class */
public class FlightPredicate implements java.util.function.Predicate {
    public Flight flight;

    public FlightPredicate(Flight flight) {
        this.flight = flight;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.flight.vertical.is623();
    }
}
