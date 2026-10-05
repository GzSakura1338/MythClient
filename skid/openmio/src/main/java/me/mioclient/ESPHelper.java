package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ESPHelper.class */
public final class ESPHelper {
    public static final String string = "dev.tr7zw.entityculling.versionless.EntityCullingVersionlessBase";
    public static final ESPHelper_3 eSPHelper_3;
    public static boolean flag;
    public static boolean flag2;

    public static void do1104() {
        if (flag || eSPHelper_3 == null) {
            return;
        }
        flag = true;
        flag2 = eSPHelper_3.is1665();
        eSPHelper_3.do1666(false);
    }

    public static void do1105() {
        if (!flag || eSPHelper_3 == null) {
            return;
        }
        flag = false;
        eSPHelper_3.do1666(flag2);
    }

    static {
        ESPHelper_2 eSPHelper_2 = null;
        try {
            Class.forName("dev.tr7zw.entityculling.versionless.EntityCullingVersionlessBase");
            eSPHelper_2 = new ESPHelper_2();
        } catch (ClassNotFoundException e) {
        }
        eSPHelper_3 = eSPHelper_2;
    }
}
