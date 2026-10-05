package com.jagrosh.discordipc.impl;

import java.util.Random;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:com/jagrosh/discordipc/impl/Backoff.class */
public class Backoff {
    public final long minAmount;
    public final long maxAmount;
    public long current;
    public int fails = 0;
    public final Random randGenerator = new Random();

    public Backoff(long j, long j2) {
        this.minAmount = j;
        this.maxAmount = j2;
        this.current = j;
    }

    public void reset() {
        this.fails = 0;
        this.current = this.minAmount;
    }

    public long nextDelay() {
        this.fails++;
        this.current = Math.min(this.current + ((long) (this.current * 2.0d * rand01())), this.maxAmount);
        return this.current;
    }

    public double rand01() {
        return this.randGenerator.nextDouble();
    }
}
