package me.mioclient;

import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/NewChunksHelper_4.class */
public class NewChunksHelper_4 {
    public static final Lock lock = new ReentrantLock();

    public static void do2149(java.lang.Runnable runnable) {
        do2151(lock, runnable);
    }

    public static java.lang.Runnable getRunnable2150(java.lang.Runnable runnable) {
        return () -> {
            do2149(runnable);
        };
    }

    public static void do2151(Lock lock2, java.lang.Runnable runnable) {
        try {
            lock2.lock();
            runnable.run();
        } finally {
            lock2.unlock();
        }
    }

    public static java.lang.Runnable getRunnable2152(Lock lock2, java.lang.Runnable runnable) {
        return () -> {
            do2151(lock2, runnable);
        };
    }
}
