package me.mioclient.feature;

import java.util.ArrayList;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/feature/Scroll.class */
public class Scroll {
    public static final int num = 5;
    public final java.util.List<String> list = new ArrayList();
    public final Stopwatch stopwatch = new Stopwatch();
    public int num2 = 0;

    public void do466() {
        if (this.stopwatch.is419(1500L)) {
            this.num2++;
            this.stopwatch.reset();
        }
        if (this.num2 >= this.list.size()) {
            this.num2 = 0;
        }
    }

    public void reset() {
        this.list.clear();
        this.num2 = 0;
    }

    public java.util.List<String> getList2996() {
        if (this.list.size() <= 5) {
            return this.list;
        }
        ArrayList arrayList = new ArrayList();
        for (int i = 0; i < 5; i++) {
            arrayList.add(this.list.get((i + this.num2) % this.list.size()));
        }
        return arrayList;
    }

    public int get1296() {
        return this.num2;
    }

    public java.util.List<String> getList2997() {
        return this.list;
    }
}
