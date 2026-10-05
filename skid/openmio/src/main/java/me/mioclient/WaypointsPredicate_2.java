package me.mioclient;

import me.mioclient.module.render.Waypoints;

/* loaded from: mio-yarn.jar:me/mioclient/WaypointsPredicate_2.class */
public class WaypointsPredicate_2 implements java.util.function.Predicate {
    public Waypoints waypoints;

    public WaypointsPredicate_2(Waypoints waypoints) {
        this.waypoints = waypoints;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.waypoints.name.getValue().booleanValue();
    }
}
