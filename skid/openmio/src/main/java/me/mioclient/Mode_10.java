package me.mioclient;

import java.util.ArrayList;
import java.util.List;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Mode_10.class */
public enum Mode_10 {
    WINTER(1, 2, 12),
    SPRING(3, 4, 5),
    SUMMER(6, 7, 8),
    AUTUMN(9, 10, 11);

    public final List<Integer> list = new ArrayList();

    Mode_10(int... iArr) {
        for (int i : iArr) {
            this.list.add(Integer.valueOf(i));
        }
    }

    public boolean is2576(int i) {
        return this.list.contains(Integer.valueOf(i));
    }

    public static Mode_10 getMode_102577(int i) {
        for (Mode_10 mode_10 : values()) {
            if (mode_10.is2576(i)) {
                return mode_10;
            }
        }
        throw new IllegalArgumentException("Unknown month");
    }
}
