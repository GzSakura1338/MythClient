package me.mioclient;

import me.mioclient.module.player.AutoCraft;

/* loaded from: mio-yarn.jar:me/mioclient/AutoCraftPredicate_3.class */
public class AutoCraftPredicate_3 implements java.util.function.Predicate {
    public AutoCraft autoCraft;

    public AutoCraftPredicate_3(AutoCraft autoCraft) {
        this.autoCraft = autoCraft;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.autoCraft.drop.getValue() == AutoCraft.AutoCraftMode.None;
    }
}
