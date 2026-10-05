package me.mioclient;

import me.mioclient.module.render.NameTags;

/* loaded from: mio-yarn.jar:me/mioclient/NameTagsPredicate.class */
public class NameTagsPredicate implements java.util.function.Predicate {
    public NameTags nameTags;

    public NameTagsPredicate(NameTags nameTags) {
        this.nameTags = nameTags;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.nameTags.info.is623();
    }
}
