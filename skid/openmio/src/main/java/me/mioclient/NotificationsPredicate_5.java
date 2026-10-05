package me.mioclient;

import me.mioclient.module.client.Notifications;

/* loaded from: mio-yarn.jar:me/mioclient/NotificationsPredicate_5.class */
public class NotificationsPredicate_5 implements java.util.function.Predicate {
    public Notifications notifications;

    public NotificationsPredicate_5(Notifications notifications) {
        this.notifications = notifications;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.notifications.totemPops.is623();
    }
}
