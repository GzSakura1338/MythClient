package me.mioclient;

import java.nio.charset.StandardCharsets;
import java.util.Base64;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Helper_26.class */
public final class Helper_26 {
    public static final byte[] byteArr = {7, 77, -33, 27, 63, -73, 114, -122, 16, 41, 4, Byte.MIN_VALUE, -51, -25, 42, -49, 74, -112, -93, 12, 96, 119, -106, -11, -26, 15, -109, -37, 122, -104, -16, 62, 59, 101, -102, -81, 3, 47, -68, -77, -1, 24, 90, 100, -23, -109, -107, -86, -126, 64, -115, -66, Byte.MIN_VALUE, -96, 75, 28, -95, 50, -118, -24, 80, -126, -29, 2, 77, 26, 122, -32, -2, 94, -100, -27, -46, 102, -103, -36, 102, -55, -32, -23, 79, -70, 36, -50, 40, -60, -50, -56, -10, 48, 83, 89, 53, 91, -26, -73};

    public Helper_26() {
        throw new AssertionError();
    }

    public static byte[] getByteArray2999(byte[] bArr) {
        byte[] bArr2 = new byte[bArr.length];
        for (int i = 0; i < bArr.length; i++) {
            bArr2[i] = (byte) (bArr[i] ^ byteArr[i % byteArr.length]);
        }
        return bArr2;
    }

    public static String getString3000(String str) {
        try {
            return new String(Base64.getEncoder().encode(getByteArray2999(str.getBytes(StandardCharsets.UTF_8))), StandardCharsets.UTF_8);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    public static String getString3001(String str) {
        try {
            return new String(getByteArray2999(Base64.getDecoder().decode(str)), StandardCharsets.UTF_8);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }
}
