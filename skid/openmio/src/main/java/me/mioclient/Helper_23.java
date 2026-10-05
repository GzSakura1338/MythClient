package me.mioclient;

import java.util.ArrayList;
import java.util.List;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Helper_23.class */
public class Helper_23 {
    public static List<String> getList2521(String str, double d) {
        ArrayList arrayList = new ArrayList();
        StringBuilder sb = new StringBuilder();
        char c = 65535;
        for (int i = 0; i < str.length(); i++) {
            char charAt = str.charAt(i);
            if (charAt == 167 && i < str.length() - 1) {
                c = str.charAt(i + 1);
            }
            if (FontsSearchHelper4.fontsSearchHelper4.get1316(new ArgumentTypeHelper().getArgumentTypeHelper2904(charAt).getArgumentTypeHelper2919(sb.toString()).getString2921("\u0001\u0001")) < d) {
                sb.append(charAt);
            } else {
                arrayList.add(sb.toString());
                sb = new StringBuilder().append((char) 167).append(c).append(charAt);
            }
        }
        if (sb.length() > 0) {
            arrayList.add(sb.toString());
        }
        return arrayList;
    }

    public static String[] getStringArray2522(String str) {
        ArrayList arrayList = new ArrayList(1);
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < str.length(); i++) {
            char charAt = str.charAt(i);
            if (charAt == '\n') {
                arrayList.add(sb.toString());
                sb = new StringBuilder();
            } else {
                sb.append(charAt);
            }
        }
        arrayList.add(sb.toString());
        return (String[]) arrayList.toArray(new String[0]);
    }

    public static Character getCharacter2523(String str) {
        char c = ' ';
        for (int i = 0; i < str.length(); i++) {
            if (str.charAt(i) == 167 && i + 1 < str.length()) {
                c = str.charAt(i + 1);
            }
        }
        return Character.valueOf(c);
    }
}
