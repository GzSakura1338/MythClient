package me.mioclient;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ObstaclePasserHelper_2.class */
public final class ObstaclePasserHelper_2 {
    public final List<Long> list;
    public final int num;
    public final int num2;

    public ObstaclePasserHelper_2(int i) {
        this(1000, i);
    }

    public ObstaclePasserHelper_2(int i, int i2) {
        this.list = Collections.synchronizedList(new ArrayList());
        this.num = i;
        this.num2 = i2;
    }

    public void do976() {
        this.list.add(Long.valueOf(System.currentTimeMillis()));
    }

    public boolean is977() {
        return this.list.size() > this.num2;
    }

    public void do466() {
        this.list.removeIf(l -> {
            return System.currentTimeMillis() > l.longValue() + ((long) this.num);
        });
    }

    public void do978() {
        this.list.clear();
    }
}
