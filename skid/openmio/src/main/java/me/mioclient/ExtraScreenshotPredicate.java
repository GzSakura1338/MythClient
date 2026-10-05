package me.mioclient;

import me.mioclient.module.misc.ExtraScreenshot;

/* loaded from: mio-yarn.jar:me/mioclient/ExtraScreenshotPredicate.class */
public class ExtraScreenshotPredicate implements java.util.function.Predicate {
    public ExtraScreenshot extraScreenshot;

    public ExtraScreenshotPredicate(ExtraScreenshot extraScreenshot) {
        this.extraScreenshot = extraScreenshot;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.extraScreenshot.mode.getValue() != ExtraScreenshot.ExtraScreenshotMode.NONE;
    }
}
