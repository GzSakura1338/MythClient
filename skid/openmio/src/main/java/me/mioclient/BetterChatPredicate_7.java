package me.mioclient;

import me.mioclient.module.misc.BetterChat;

/* loaded from: mio-yarn.jar:me/mioclient/BetterChatPredicate_7.class */
public class BetterChatPredicate_7 implements java.util.function.Predicate {
    public BetterChat betterChat;

    public BetterChatPredicate_7(BetterChat betterChat) {
        this.betterChat = betterChat;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.betterChat.animation.is623();
    }
}
