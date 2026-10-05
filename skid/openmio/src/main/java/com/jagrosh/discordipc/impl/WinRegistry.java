package com.jagrosh.discordipc.impl;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.prefs.Preferences;
import net.lenni0451.reflect.Methods;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:com/jagrosh/discordipc/impl/WinRegistry.class */
public class WinRegistry {
    public static final int HKEY_CURRENT_USER = -2147483647;
    public static final int HKEY_LOCAL_MACHINE = -2147483646;
    public static final int REG_SUCCESS = 0;
    public static final int KEY_ALL_ACCESS = 983103;
    public static final int KEY_READ = 131097;
    public static final Preferences userRoot = Preferences.userRoot();
    public static final Preferences systemRoot = Preferences.systemRoot();
    public static final Class<? extends Preferences> userClass = userRoot.getClass();
    public static final Method regOpenKey;
    public static final Method regCloseKey;
    public static final Method regQueryValueEx;
    public static final Method regEnumValue;
    public static final Method regQueryInfoKey;
    public static final Method regEnumKeyEx;
    public static final Method regCreateKeyEx;
    public static final Method regSetValueEx;
    public static final Method regDeleteKey;
    public static final Method regDeleteValue;
    public static final float javaSpec;

    public static String readString(int i, String str, String str2) {
        if (i == -2147483646) {
            return readString(systemRoot, i, str, str2);
        }
        if (i == -2147483647) {
            return readString(userRoot, i, str, str2);
        }
        throw new IllegalArgumentException("hkey=" + i);
    }

    public static String readString(long j, String str, String str2) {
        if (j == -2147483646) {
            return readString(systemRoot, j, str, str2);
        }
        if (j == -2147483647L) {
            return readString(userRoot, j, str, str2);
        }
        throw new IllegalArgumentException("hkey=" + j);
    }

    public static Map<String, String> readStringValues(int i, String str) {
        if (i == -2147483646) {
            return readStringValues(systemRoot, i, str);
        }
        if (i == -2147483647) {
            return readStringValues(userRoot, i, str);
        }
        throw new IllegalArgumentException("hkey=" + i);
    }

    public static List<String> readStringSubKeys(int i, String str) {
        if (i == -2147483646) {
            return readStringSubKeys(systemRoot, i, str);
        }
        if (i == -2147483647) {
            return readStringSubKeys(userRoot, i, str);
        }
        throw new IllegalArgumentException("hkey=" + i);
    }

    public static void createKey(int i, String str) {
        int[] createKey;
        if (i == -2147483646) {
            createKey = createKey(systemRoot, i, str);
            Methods.invoke(systemRoot, regCloseKey, new Object[]{Integer.valueOf(createKey[0])});
        } else {
            if (i != -2147483647) {
                throw new IllegalArgumentException("hkey=" + i);
            }
            createKey = createKey(userRoot, i, str);
            Methods.invoke(userRoot, regCloseKey, new Object[]{Integer.valueOf(createKey[0])});
        }
        if (createKey[1] != 0) {
            throw new IllegalArgumentException("rc=" + createKey[1] + "  key=" + str);
        }
    }

    public static void createKey(long j, String str) {
        long[] createKey;
        if (j == -2147483646) {
            createKey = createKey(systemRoot, j, str);
            Methods.invoke(systemRoot, regCloseKey, new Object[]{Long.valueOf(createKey[0])});
        } else {
            if (j != -2147483647L) {
                throw new IllegalArgumentException("hkey=" + j);
            }
            createKey = createKey(userRoot, j, str);
            Methods.invoke(userRoot, regCloseKey, new Object[]{Long.valueOf(createKey[0])});
        }
        if (createKey[1] != 0) {
            throw new IllegalArgumentException("rc=" + createKey[1] + "  key=" + str);
        }
    }

    public static void writeStringValue(int i, String str, String str2, String str3) {
        if (i == -2147483646) {
            writeStringValue(systemRoot, i, str, str2, str3);
        } else {
            if (i != -2147483647) {
                throw new IllegalArgumentException("hkey=" + i);
            }
            writeStringValue(userRoot, i, str, str2, str3);
        }
    }

    public static void writeStringValue(long j, String str, String str2, String str3) {
        if (j == -2147483646) {
            writeStringValue(systemRoot, j, str, str2, str3);
        } else {
            if (j != -2147483647L) {
                throw new IllegalArgumentException("hkey=" + j);
            }
            writeStringValue(userRoot, j, str, str2, str3);
        }
    }

    public static void deleteKey(int i, String str) {
        int i2 = -1;
        if (i == -2147483646) {
            i2 = deleteKey(systemRoot, i, str);
        } else if (i == -2147483647) {
            i2 = deleteKey(userRoot, i, str);
        }
        if (i2 != 0) {
            throw new IllegalArgumentException("rc=" + i2 + "  key=" + str);
        }
    }

    public static void deleteValue(int i, String str, String str2) {
        int i2 = -1;
        if (i == -2147483646) {
            i2 = deleteValue(systemRoot, i, str, str2);
        } else if (i == -2147483647) {
            i2 = deleteValue(userRoot, i, str, str2);
        }
        if (i2 != 0) {
            throw new IllegalArgumentException("rc=" + i2 + "  key=" + str + "  value=" + str2);
        }
    }

    public static int deleteValue(Preferences preferences, int i, String str, String str2) {
        int[] iArr = (int[]) Methods.invoke(preferences, regOpenKey, new Object[]{Integer.valueOf(i), toCstr(str), Integer.valueOf(KEY_ALL_ACCESS)});
        if (iArr[1] != 0) {
            return iArr[1];
        }
        int intValue = ((Integer) Methods.invoke(preferences, regDeleteValue, new Object[]{Integer.valueOf(iArr[0]), toCstr(str2)})).intValue();
        Methods.invoke(preferences, regCloseKey, new Object[]{Integer.valueOf(iArr[0])});
        return intValue;
    }

    public static int deleteKey(Preferences preferences, int i, String str) {
        return ((Integer) Methods.invoke(preferences, regDeleteKey, new Object[]{Integer.valueOf(i), toCstr(str)})).intValue();
    }

    public static String readString(Preferences preferences, int i, String str, String str2) {
        int[] iArr = (int[]) Methods.invoke(preferences, regOpenKey, new Object[]{Integer.valueOf(i), toCstr(str), Integer.valueOf(KEY_READ)});
        if (iArr[1] != 0) {
            return null;
        }
        byte[] bArr = (byte[]) Methods.invoke(preferences, regQueryValueEx, new Object[]{Integer.valueOf(iArr[0]), toCstr(str2)});
        Methods.invoke(preferences, regCloseKey, new Object[]{Integer.valueOf(iArr[0])});
        if (bArr != null) {
            return new String(bArr).trim();
        }
        return null;
    }

    public static String readString(Preferences preferences, long j, String str, String str2) {
        long[] jArr = (long[]) Methods.invoke(preferences, regOpenKey, new Object[]{Long.valueOf(j), toCstr(str), Integer.valueOf(KEY_READ)});
        if (jArr[1] != 0) {
            return null;
        }
        byte[] bArr = (byte[]) Methods.invoke(preferences, regQueryValueEx, new Object[]{Long.valueOf(jArr[0]), toCstr(str2)});
        Methods.invoke(preferences, regCloseKey, new Object[]{Long.valueOf(jArr[0])});
        if (bArr != null) {
            return new String(bArr).trim();
        }
        return null;
    }

    public static Map<String, String> readStringValues(Preferences preferences, int i, String str) {
        HashMap hashMap = new HashMap();
        int[] iArr = (int[]) Methods.invoke(preferences, regOpenKey, new Object[]{Integer.valueOf(i), toCstr(str), Integer.valueOf(KEY_READ)});
        if (iArr[1] != 0) {
            return null;
        }
        int[] iArr2 = (int[]) Methods.invoke(preferences, regQueryInfoKey, new Object[]{Integer.valueOf(iArr[0])});
        int i2 = iArr2[0];
        int i3 = iArr2[3];
        for (int i4 = 0; i4 < i2; i4++) {
            byte[] bArr = (byte[]) Methods.invoke(preferences, regEnumValue, new Object[]{Integer.valueOf(iArr[0]), Integer.valueOf(i4), Integer.valueOf(i3 + 1)});
            hashMap.put(new String(bArr).trim(), readString(i, str, new String(bArr)));
        }
        Methods.invoke(preferences, regCloseKey, new Object[]{Integer.valueOf(iArr[0])});
        return hashMap;
    }

    public static List<String> readStringSubKeys(Preferences preferences, int i, String str) {
        ArrayList arrayList = new ArrayList();
        int[] iArr = (int[]) Methods.invoke(preferences, regOpenKey, new Object[]{Integer.valueOf(i), toCstr(str), Integer.valueOf(KEY_READ)});
        if (iArr[1] != 0) {
            return null;
        }
        int[] iArr2 = (int[]) Methods.invoke(preferences, regQueryInfoKey, new Object[]{Integer.valueOf(iArr[0])});
        int i2 = iArr2[0];
        int i3 = iArr2[3];
        for (int i4 = 0; i4 < i2; i4++) {
            arrayList.add(new String((byte[]) Methods.invoke(preferences, regEnumKeyEx, new Object[]{Integer.valueOf(iArr[0]), Integer.valueOf(i4), Integer.valueOf(i3 + 1)})).trim());
        }
        Methods.invoke(preferences, regCloseKey, new Object[]{Integer.valueOf(iArr[0])});
        return arrayList;
    }

    public static int[] createKey(Preferences preferences, int i, String str) {
        return (int[]) Methods.invoke(preferences, regCreateKeyEx, new Object[]{Integer.valueOf(i), toCstr(str)});
    }

    public static long[] createKey(Preferences preferences, long j, String str) {
        return (long[]) Methods.invoke(preferences, regCreateKeyEx, new Object[]{Long.valueOf(j), toCstr(str)});
    }

    public static void writeStringValue(Preferences preferences, int i, String str, String str2, String str3) {
        int[] iArr = (int[]) Methods.invoke(preferences, regOpenKey, new Object[]{Integer.valueOf(i), toCstr(str), Integer.valueOf(KEY_ALL_ACCESS)});
        Methods.invoke(preferences, regSetValueEx, new Object[]{Integer.valueOf(iArr[0]), toCstr(str2), toCstr(str3)});
        Methods.invoke(preferences, regCloseKey, new Object[]{Integer.valueOf(iArr[0])});
    }

    public static void writeStringValue(Preferences preferences, long j, String str, String str2, String str3) {
        long[] jArr = (long[]) Methods.invoke(preferences, regOpenKey, new Object[]{Long.valueOf(j), toCstr(str), Integer.valueOf(KEY_ALL_ACCESS)});
        Methods.invoke(preferences, regSetValueEx, new Object[]{Long.valueOf(jArr[0]), toCstr(str2), toCstr(str3)});
        Methods.invoke(preferences, regCloseKey, new Object[]{Long.valueOf(jArr[0])});
    }

    public static byte[] toCstr(String str) {
        byte[] bArr = new byte[str.length() + 1];
        for (int i = 0; i < str.length(); i++) {
            bArr[i] = (byte) str.charAt(i);
        }
        bArr[str.length()] = 0;
        return bArr;
    }

    static {
        try {
            javaSpec = Float.parseFloat(System.getProperty("java.specification.version"));
            Class<? extends Preferences> cls = userClass;
            Class[] clsArr = new Class[3];
            clsArr[0] = javaSpec >= 11.0f ? Long.TYPE : Integer.TYPE;
            clsArr[1] = byte[].class;
            clsArr[2] = Integer.TYPE;
            regOpenKey = Methods.getDeclaredMethod(cls, "WindowsRegOpenKey", clsArr);
            Class<? extends Preferences> cls2 = userClass;
            Class[] clsArr2 = new Class[1];
            clsArr2[0] = javaSpec >= 11.0f ? Long.TYPE : Integer.TYPE;
            regCloseKey = Methods.getDeclaredMethod(cls2, "WindowsRegCloseKey", clsArr2);
            Class<? extends Preferences> cls3 = userClass;
            Class[] clsArr3 = new Class[2];
            clsArr3[0] = javaSpec >= 11.0f ? Long.TYPE : Integer.TYPE;
            clsArr3[1] = byte[].class;
            regQueryValueEx = Methods.getDeclaredMethod(cls3, "WindowsRegQueryValueEx", clsArr3);
            Class<? extends Preferences> cls4 = userClass;
            Class[] clsArr4 = new Class[3];
            clsArr4[0] = javaSpec >= 11.0f ? Long.TYPE : Integer.TYPE;
            clsArr4[1] = Integer.TYPE;
            clsArr4[2] = Integer.TYPE;
            regEnumValue = Methods.getDeclaredMethod(cls4, "WindowsRegEnumValue", clsArr4);
            Class<? extends Preferences> cls5 = userClass;
            Class[] clsArr5 = new Class[1];
            clsArr5[0] = javaSpec >= 11.0f ? Long.TYPE : Integer.TYPE;
            regQueryInfoKey = Methods.getDeclaredMethod(cls5, "WindowsRegQueryInfoKey1", clsArr5);
            Class<? extends Preferences> cls6 = userClass;
            Class[] clsArr6 = new Class[3];
            clsArr6[0] = javaSpec >= 11.0f ? Long.TYPE : Integer.TYPE;
            clsArr6[1] = Integer.TYPE;
            clsArr6[2] = Integer.TYPE;
            regEnumKeyEx = Methods.getDeclaredMethod(cls6, "WindowsRegEnumKeyEx", clsArr6);
            Class<? extends Preferences> cls7 = userClass;
            Class[] clsArr7 = new Class[2];
            clsArr7[0] = javaSpec >= 11.0f ? Long.TYPE : Integer.TYPE;
            clsArr7[1] = byte[].class;
            regCreateKeyEx = Methods.getDeclaredMethod(cls7, "WindowsRegCreateKeyEx", clsArr7);
            Class<? extends Preferences> cls8 = userClass;
            Class[] clsArr8 = new Class[3];
            clsArr8[0] = javaSpec >= 11.0f ? Long.TYPE : Integer.TYPE;
            clsArr8[1] = byte[].class;
            clsArr8[2] = byte[].class;
            regSetValueEx = Methods.getDeclaredMethod(cls8, "WindowsRegSetValueEx", clsArr8);
            Class<? extends Preferences> cls9 = userClass;
            Class[] clsArr9 = new Class[2];
            clsArr9[0] = javaSpec >= 11.0f ? Long.TYPE : Integer.TYPE;
            clsArr9[1] = byte[].class;
            regDeleteValue = Methods.getDeclaredMethod(cls9, "WindowsRegDeleteValue", clsArr9);
            Class<? extends Preferences> cls10 = userClass;
            Class[] clsArr10 = new Class[2];
            clsArr10[0] = javaSpec >= 11.0f ? Long.TYPE : Integer.TYPE;
            clsArr10[1] = byte[].class;
            regDeleteKey = Methods.getDeclaredMethod(cls10, "WindowsRegDeleteKey", clsArr10);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
