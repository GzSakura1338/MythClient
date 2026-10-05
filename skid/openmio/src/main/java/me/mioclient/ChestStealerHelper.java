package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ChestStealerHelper.class */
public final class ChestStealerHelper {
    public final int num;
    public final int num2;
    public int num3;

    public ChestStealerHelper(int i, int i2, int i3) {
        this.num = i;
        this.num3 = i3;
        this.num2 = i2;
    }

    public int get499() {
        return this.num;
    }

    public int get3063() {
        return this.num2;
    }

    public int get3064() {
        return this.num3;
    }

    public void do3065(int i) {
        this.num3 = Math.min(i, get3063());
    }
}
