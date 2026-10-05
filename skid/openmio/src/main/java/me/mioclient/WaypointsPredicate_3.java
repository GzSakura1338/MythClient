package me.mioclient;

import me.mioclient.module.render.Waypoints;

/* loaded from: mio-yarn.jar:me/mioclient/WaypointsPredicate_3.class */
public class WaypointsPredicate_3 implements java.util.function.Predicate {
    public Waypoints waypoints;

    public WaypointsPredicate_3(Waypoints waypoints) {
        this.waypoints = waypoints;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.waypoints.name.getValue().booleanValue();
    }
}
