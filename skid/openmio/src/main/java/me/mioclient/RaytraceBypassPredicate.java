package me.mioclient;

import me.mioclient.module.exploit.RaytraceBypass;

/* loaded from: mio-yarn.jar:me/mioclient/RaytraceBypassPredicate.class */
public class RaytraceBypassPredicate implements java.util.function.Predicate {
    public RaytraceBypass raytraceBypass;

    public RaytraceBypassPredicate(RaytraceBypass raytraceBypass) {
        this.raytraceBypass = raytraceBypass;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.raytraceBypass.rotations.getValue() == RaytraceBypass.RaytraceBypassMode.MOTION;
    }
}
