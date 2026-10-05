package me.mioclient;

import java.util.Random;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/RandomHelper.class */
public class RandomHelper {
    public Mode_7 mode_7;
    public int[][] intArr;
    public int[][][] intArr2;
    public Random random = new Random();

    public RandomHelper() {
        do1167();
    }

    /* JADX WARN: Multi-variable type inference failed */
    /* JADX WARN: Type inference failed for: r1v3, types: [int[][], int[][][]] */
    public void do1167() {
        this.intArr = new int[4][2];
        this.intArr2 = new int[][][]{new int[][]{new int[]{0, 0}, new int[]{0, 0}, new int[]{0, 0}, new int[]{0, 0}}, new int[][]{new int[]{0, -1}, new int[]{0, 0}, new int[]{-1, 0}, new int[]{-1, 1}}, new int[][]{new int[]{0, -1}, new int[]{0, 0}, new int[]{1, 0}, new int[]{1, 1}}, new int[][]{new int[]{0, -1}, new int[]{0, 0}, new int[]{0, 1}, new int[]{0, 2}}, new int[][]{new int[]{-1, 0}, new int[]{0, 0}, new int[]{1, 0}, new int[]{0, 1}}, new int[][]{new int[]{0, 0}, new int[]{1, 0}, new int[]{0, 1}, new int[]{1, 1}}, new int[][]{new int[]{-1, -1}, new int[]{0, -1}, new int[]{0, 0}, new int[]{0, 1}}, new int[][]{new int[]{1, -1}, new int[]{0, -1}, new int[]{0, 0}, new int[]{0, 1}}};
        do1168(Mode_7.NoShape);
    }

    public void do1168(Mode_7 mode_7) {
        for (int i = 0; i < 4; i++) {
            System.arraycopy(this.intArr2[mode_7.ordinal()][i], 0, this.intArr[i], 0, 2);
        }
        this.mode_7 = mode_7;
    }

    public void do1169(int i, int i2) {
        this.intArr[i][0] = i2;
    }

    public void do1170(int i, int i2) {
        this.intArr[i][1] = i2;
    }

    public int get1171(int i) {
        return this.intArr[i][0];
    }

    public int get1172(int i) {
        return this.intArr[i][1];
    }

    public Mode_7 getMode_71173() {
        return this.mode_7;
    }

    public void do1174() {
        do1168(Mode_7.values()[(Math.abs(this.random.nextInt()) % 7) + 1]);
    }

    public int get1175() {
        int i = this.intArr[0][0];
        for (int i2 = 0; i2 < 4; i2++) {
            i = Math.min(i, this.intArr[i2][0]);
        }
        return i;
    }

    public int get1176() {
        int i = this.intArr[0][1];
        for (int i2 = 0; i2 < 4; i2++) {
            i = Math.min(i, this.intArr[i2][1]);
        }
        return i;
    }

    public RandomHelper getRandomHelper1177() {
        if (this.mode_7 == Mode_7.SquareShape) {
            return this;
        }
        RandomHelper randomHelper = new RandomHelper();
        randomHelper.mode_7 = this.mode_7;
        for (int i = 0; i < 4; i++) {
            randomHelper.do1169(i, get1172(i));
            randomHelper.do1170(i, -get1171(i));
        }
        return randomHelper;
    }

    public RandomHelper getRandomHelper1178() {
        if (this.mode_7 == Mode_7.SquareShape) {
            return this;
        }
        RandomHelper randomHelper = new RandomHelper();
        randomHelper.mode_7 = this.mode_7;
        for (int i = 0; i < 4; i++) {
            randomHelper.do1169(i, -get1172(i));
            randomHelper.do1170(i, get1171(i));
        }
        return randomHelper;
    }
}
