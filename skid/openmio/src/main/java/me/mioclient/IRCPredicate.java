package me.mioclient;

import me.mioclient.module.client.IRC;

/* loaded from: mio-yarn.jar:me/mioclient/IRCPredicate.class */
public class IRCPredicate implements java.util.function.Predicate {
    public IRC iRC;

    public IRCPredicate(IRC irc) {
        this.iRC = irc;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.iRC.chat.is623() && this.iRC.chatSound.is623();
    }
}
