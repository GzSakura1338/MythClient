package me.mioclient;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.regex.Matcher;

/* loaded from: mio-yarn.jar:me/mioclient/ArgumentTypeHelper.class */
public final class ArgumentTypeHelper {
    public static final char ch2 = 2;
    public final List<String> list = new ArrayList();
    public static final char ch = 1;
    public static final String string = String.valueOf(ch);

    public ArgumentTypeHelper getArgumentTypeHelper2902(boolean z) {
        do2920(String.valueOf(z));
        return this;
    }

    public ArgumentTypeHelper getArgumentTypeHelper2903(byte b) {
        do2920(String.valueOf((int) b));
        return this;
    }

    public ArgumentTypeHelper getArgumentTypeHelper2904(char c) {
        do2920(String.valueOf(c));
        return this;
    }

    public ArgumentTypeHelper getArgumentTypeHelper2905(short s) {
        do2920(String.valueOf((int) s));
        return this;
    }

    public ArgumentTypeHelper getArgumentTypeHelper2906(int i) {
        do2920(String.valueOf(i));
        return this;
    }

    public ArgumentTypeHelper getArgumentTypeHelper2907(long j) {
        do2920(String.valueOf(j));
        return this;
    }

    public ArgumentTypeHelper getArgumentTypeHelper2908(double d) {
        do2920(String.valueOf(d));
        return this;
    }

    public ArgumentTypeHelper getArgumentTypeHelper2909(float f) {
        do2920(String.valueOf(f));
        return this;
    }

    public ArgumentTypeHelper getArgumentTypeHelper2910(boolean[] zArr) {
        do2920(Arrays.toString(zArr));
        return this;
    }

    public ArgumentTypeHelper getArgumentTypeHelper2911(byte[] bArr) {
        do2920(Arrays.toString(bArr));
        return this;
    }

    public ArgumentTypeHelper getArgumentTypeHelper2912(char[] cArr) {
        do2920(Arrays.toString(cArr));
        return this;
    }

    public ArgumentTypeHelper getArgumentTypeHelper2913(short[] sArr) {
        do2920(Arrays.toString(sArr));
        return this;
    }

    public ArgumentTypeHelper getArgumentTypeHelper2914(int[] iArr) {
        do2920(Arrays.toString(iArr));
        return this;
    }

    public ArgumentTypeHelper getArgumentTypeHelper2915(long[] jArr) {
        do2920(Arrays.toString(jArr));
        return this;
    }

    public ArgumentTypeHelper getArgumentTypeHelper2916(double[] dArr) {
        do2920(Arrays.toString(dArr));
        return this;
    }

    public ArgumentTypeHelper getArgumentTypeHelper2917(float[] fArr) {
        do2920(Arrays.toString(fArr));
        return this;
    }

    public ArgumentTypeHelper getArgumentTypeHelper2918(Object[] objArr) {
        do2920(Arrays.toString(objArr));
        return this;
    }

    public ArgumentTypeHelper getArgumentTypeHelper2919(Object obj) {
        do2920(String.valueOf(obj));
        return this;
    }

    public void do2920(String str) {
        this.list.add(0, str);
    }

    public String getString2921(String str) {
        return getString2922(str, this.list);
    }

    public static String getString2922(String str, List<String> list) {
        for (int i = 0; str.indexOf(1) >= 0 && i != list.size(); i++) {
            str = str.replaceFirst(string, Matcher.quoteReplacement(list.get(i)));
        }
        return str;
    }
}
