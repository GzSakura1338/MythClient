package me.mioclient;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Queue;
import me.mioclient.event.Listen;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/TooltipsSearchHelper4_2.class */
public class TooltipsSearchHelper4_2 extends ModuleListHelper<TooltipsSearchHelper4_2.Inner, List<TooltipsSearchHelper4_2.Inner>> implements SearchHelper_4 {
    public final Queue<Inner> queue;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/TooltipsSearchHelper4_2$Inner.class */
    public static class Inner {
        public final java.lang.Runnable runnable;
        public int num;

        public Inner(java.lang.Runnable runnable, int i) {
            this.runnable = runnable;
            this.num = i;
        }

        public java.lang.Runnable getRunnable2092() {
            return this.runnable;
        }

        public int get2093() {
            return this.num;
        }

        public boolean is2094() {
            int i = this.num;
            this.num = i - 1;
            if (i >= 0) {
                return false;
            }
            this.runnable.run();
            return true;
        }
    }

    public TooltipsSearchHelper4_2() {
        super(Collections.synchronizedList(new ArrayList()));
        this.queue = new ArrayDeque();
        baritoneHelper.do1796(this);
    }

    @Listen(get219= Helper_7.num4)
    public void onEvent(HoleSnapEvent holeSnapEvent) {
        synchronized (((List) this.registry)) {
            while (!this.queue.isEmpty()) {
                ((List) this.registry).add(this.queue.poll());
            }
            ((List<Inner>) this.registry).removeIf((v0) -> {
                return v0.is2094();
            });
        }
    }

    public void do164(java.lang.Runnable runnable, int i) {
        Inner inner = new Inner(runnable, i);
        if (minecraftClient.isOnThread()) {
            register(inner);
            return;
        }
        minecraftClient.executeSync(() -> {
            register(inner);
        });
    }

    @Override // me.mioclient.Helper_9
    /* renamed from: is165, reason: merged with bridge method [inline-methods] */
    public boolean register(Inner inner) {
        boolean add;
        synchronized (((List) this.registry)) {
            add = this.queue.add(inner);
        }
        return add;
    }

    @Override // me.mioclient.Helper_9
    /* renamed from: is166, reason: merged with bridge method [inline-methods] */
    public boolean unregister(Inner inner) {
        throw new UnsupportedOperationException();
    }
}
