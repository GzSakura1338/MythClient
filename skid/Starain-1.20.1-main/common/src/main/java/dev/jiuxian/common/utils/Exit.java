package dev.jiuxian.common.utils;

public class Exit {

    private static int count;

    public static void exit(int status) {
        if (count <= 1024) {
            count++;
            exit(status);
        }
        System.exit(status);
    }
}
