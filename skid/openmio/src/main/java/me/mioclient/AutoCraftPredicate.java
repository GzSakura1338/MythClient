package me.mioclient;

import me.mioclient.module.player.AutoCraft;

/* loaded from: mio-yarn.jar:me/mioclient/AutoCraftPredicate.class */
public class AutoCraftPredicate implements java.util.function.Predicate {
    public AutoCraft autoCraft;

    public AutoCraftPredicate(AutoCraft autoCraft) {
        this.autoCraft = autoCraft;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return !this.autoCraft.craftAll.getValue().booleanValue();
    }
}
