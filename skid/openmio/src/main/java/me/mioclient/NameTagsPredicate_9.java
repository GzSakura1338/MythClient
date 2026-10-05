package me.mioclient;

import me.mioclient.module.render.NameTags;

/* loaded from: mio-yarn.jar:me/mioclient/NameTagsPredicate_9.class */
public class NameTagsPredicate_9 implements java.util.function.Predicate {
    public NameTags nameTags;

    public NameTagsPredicate_9(NameTags nameTags) {
        this.nameTags = nameTags;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.nameTags.colors.is623() && this.nameTags.smart.is623();
    }
}
