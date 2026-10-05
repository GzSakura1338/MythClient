package me.mioclient;

import javax.crypto.Cipher;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ByteArrayDecoderHelper.class */
public class ByteArrayDecoderHelper {
    public static byte[] getByteArray2626(byte[] bArr) {
        try {
            Cipher cipher = Cipher.getInstance("AES/CBC/PKCS5Padding");
            cipher.init(1, ByteArrayDecoderHelper_2.getSecretKey2103(), ByteArrayDecoderHelper_2.getIvParameterSpec2104());
            byte[] doFinal = cipher.doFinal(bArr);
            do2628(doFinal);
            return doFinal;
        } catch (Exception e) { throw new java.lang.RuntimeException(e); }
    }

    public static byte[] getByteArray2627(byte[] bArr) {
        try {
            Cipher cipher = Cipher.getInstance("AES/CBC/PKCS5Padding");
            cipher.init(2, ByteArrayDecoderHelper_2.getSecretKey2103(), ByteArrayDecoderHelper_2.getIvParameterSpec2104());
            do2628(bArr);
            return cipher.doFinal(bArr);
        } catch (Exception e) { throw new java.lang.RuntimeException(e); }
    }

    public static void do2628(byte[] bArr) {
        if (bArr != null) {
            int length = bArr.length - 1;
            for (int i = 0; length > i; i++) {
                byte b = bArr[length];
                bArr[length] = bArr[i];
                bArr[i] = b;
                length--;
            }
        }
    }
}
