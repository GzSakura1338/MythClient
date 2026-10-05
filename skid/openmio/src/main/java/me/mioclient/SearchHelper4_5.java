package me.mioclient;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.function.BiConsumer;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper4_5.class */
public final class SearchHelper4_5 extends StashFinderModuleListHelper<EnumSettingHelper_2> implements SearchHelper_4 {
    public final List<CompletableFuture<?>> list = new ArrayList();
    public int num = 0;

    public void do2292(String str, BiConsumer<String, ? super Throwable> biConsumer) {
    }

    public boolean is2293() {
        Iterator<CompletableFuture<?>> it = this.list.iterator();
        while (it.hasNext()) {
            if (!it.next().isDone()) {
                return false;
            }
        }
        return true;
    }

    public boolean is2294() {
        Iterator<CompletableFuture<?>> it = this.list.iterator();
        while (it.hasNext()) {
            if (it.next().isCompletedExceptionally()) {
                return true;
            }
        }
        return false;
    }

    public int get2295() {
        return this.num;
    }
}
