package me.mioclient;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Data_3.class */
public final class Data_3 {
    public final float val;
    public final float val2;
    public final float val3;
    public final float val4;

    public Data_3(float f, float f2, float f3, float f4) {
        this.val = f;
        this.val2 = f2;
        this.val3 = f3;
        this.val4 = f4;
    }

    public static Data_3 getData_31612(float f, float f2, float f3, float f4) {
        return new Data_3(f, f2, f + f3, f2 + f4);
    }

    public float get1613() {
        return this.val3 - this.val;
    }

    public float get1614() {
        return this.val4 - this.val2;
    }

    public boolean is92(double d, double d2) {
        return d >= ((double) this.val) && d <= ((double) this.val3) && d2 >= ((double) this.val2) && d2 <= ((double) this.val4);
    }

    public boolean is1615(Data_3 data_3) {
        return (this.val3 < this.val || this.val3 > data_3.get1619()) && (this.val4 < this.val2 || this.val4 > data_3.get1620()) && ((data_3.get1621() < data_3.get1619() || data_3.get1621() > this.val) && (data_3.get1622() < data_3.get1620() || data_3.get1622() > this.val2));
    }

    public Data_3 getData_31616(float f) {
        return getData_31617(f, f);
    }

    public Data_3 getData_31617(float f, float f2) {
        return new Data_3(get1619() - f, get1620() - f2, get1621() + f, get1622() + f2);
    }

    public Data_3 getData_31618(Data_3... data_3Arr) {
        float f = get1619();
        float f2 = get1621();
        float f3 = get1620();
        float f4 = get1622();
        for (Data_3 data_3 : data_3Arr) {
            f = Math.min(f, data_3.get1619());
            f3 = Math.min(f3, data_3.get1620());
            f2 = Math.max(f2, data_3.get1621());
            f4 = Math.max(f4, data_3.get1622());
        }
        return new Data_3(f, f3, f2, f4);
    }




    public float get1619() {
        return this.val;
    }

    public float get1620() {
        return this.val2;
    }

    public float get1621() {
        return this.val3;
    }

    public float get1622() {
        return this.val4;
    }
}
