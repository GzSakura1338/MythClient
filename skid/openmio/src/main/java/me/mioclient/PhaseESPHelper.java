package me.mioclient;

import nick.Loader;

/* loaded from: mio-yarn.jar:me/mioclient/PhaseESPHelper.class */
public class PhaseESPHelper {
    public static void do1351(Object obj) {
        Loader.settings(obj);
    }

    public static void do1352(Object obj) {
        Loader.modules(obj);
    }

    public static void do1353(Object obj) {
        Loader.commands(obj);
    }

    public static int get1354(long j) {
        return Loader.getInt(j);
    }

    public static String getString1355(int i) {
        return Loader.getString(i);
    }

    public static Object getObject1356(int i) {
        return Loader.getConstant(i);
    }

    public static void do1357() {
        Loader.setup();
    }
}
