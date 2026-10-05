package me.mioclient;

import javax.crypto.SecretKey;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ByteArrayDecoderHelper_2.class */
public class ByteArrayDecoderHelper_2 {
    public static SecretKey secretKey;
    public static IvParameterSpec ivParameterSpec;

    public static void reset() {
        secretKey = null;
        ivParameterSpec = null;
    }

    public static SecretKey getSecretKey2103() {
        return secretKey;
    }

    public static IvParameterSpec getIvParameterSpec2104() {
        return ivParameterSpec;
    }

    public static void do2105(byte[] bArr, byte[] bArr2) {
        secretKey = new SecretKeySpec(bArr, "AES");
        ivParameterSpec = new IvParameterSpec(bArr2);
    }

    public static boolean is2106() {
        return secretKey != null;
    }
}
