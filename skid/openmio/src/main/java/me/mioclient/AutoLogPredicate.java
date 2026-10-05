package me.mioclient;

import me.mioclient.module.combat.AutoLog;

/* loaded from: mio-yarn.jar:me/mioclient/AutoLogPredicate.class */
public class AutoLogPredicate implements java.util.function.Predicate {
    public AutoLog autoLog;

    public AutoLogPredicate(AutoLog autoLog) {
        this.autoLog = autoLog;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.autoLog.totems.is623();
    }
}
