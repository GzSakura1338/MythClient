package com.jagrosh.discordipc.impl;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:com/jagrosh/discordipc/impl/ExtendedLong.class */
public class ExtendedLong {
    public static int hashCode(long j) {
        return (int) (j ^ (j >>> 32));
    }
}
