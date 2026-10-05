package me.mioclient;

import me.mioclient.module.render.NameTags;

/* loaded from: mio-yarn.jar:me/mioclient/NameTagsPredicate_15.class */
public class NameTagsPredicate_15 implements java.util.function.Predicate {
    public NameTags nameTags;

    public NameTagsPredicate_15(NameTags nameTags) {
        this.nameTags = nameTags;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.nameTags.colors.is623();
    }
}
