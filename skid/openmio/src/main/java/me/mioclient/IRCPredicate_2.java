package me.mioclient;

import me.mioclient.module.client.IRC;

/* loaded from: mio-yarn.jar:me/mioclient/IRCPredicate_2.class */
public class IRCPredicate_2 implements java.util.function.Predicate {
    public IRC iRC;

    public IRCPredicate_2(IRC irc) {
        this.iRC = irc;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.iRC.chat.is623() && this.iRC.chatSound.is623();
    }
}
