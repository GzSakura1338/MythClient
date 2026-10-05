package me.mioclient;

import me.mioclient.module.misc.CustomDeathText;

/* loaded from: mio-yarn.jar:me/mioclient/CustomDeathTextPredicate.class */
public class CustomDeathTextPredicate implements java.util.function.Predicate {
    public CustomDeathText customDeathText;

    public CustomDeathTextPredicate(CustomDeathText customDeathText) {
        this.customDeathText = customDeathText;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.customDeathText.mode.getValue() == CustomDeathText.CustomDeathTextMode.CUSTOM;
    }
}
