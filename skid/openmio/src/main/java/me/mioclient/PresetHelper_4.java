package me.mioclient;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/PresetHelper_4.class */
public class PresetHelper_4 {
    public static Path getPath1566(Path path, String... strArr) {
        if (path.toFile().exists()) {
            return path;
        }
        for (String str : strArr) {
            Path of = Path.of(new ArgumentTypeHelper().getArgumentTypeHelper2919(str).getArgumentTypeHelper2919(path.toString()).getString2921("\u0001\u0001"), new String[0]);
            if (of.toFile().exists()) {
                return of;
            }
        }
        return path;
    }

    public static void do1567(Path path, String str) {
        do1568(path, str.getBytes(StandardCharsets.UTF_8));
    }

    public static void do1568(Path path, byte[] bArr) {
        FileOutputStream fileOutputStream = null;
        try {
            fileOutputStream = new FileOutputStream(path.toFile());
            fileOutputStream.write(bArr);
            fileOutputStream.close();
            if (fileOutputStream != null) {
                try {
                    fileOutputStream.close();
                } catch (Exception e) {
                }
            }
        } catch (Throwable th) {
            if (fileOutputStream != null) {
                try {
                    fileOutputStream.close();
                } catch (Exception e2) {
                }
            }
            throw new java.lang.RuntimeException(th);
        }
    }

    public static byte[] getByteArray1569(Path path) {
        FileInputStream fileInputStream = null;
        try {
            fileInputStream = new FileInputStream(path.toFile());
            byte[] readAllBytes = fileInputStream.readAllBytes();
            if (fileInputStream != null) {
                try {
                    fileInputStream.close();
                } catch (Exception e) {
                }
            }
            return readAllBytes;
        } catch (Throwable th) {
            if (fileInputStream != null) {
                try {
                    fileInputStream.close();
                } catch (Exception e2) {
                }
            }
            throw new java.lang.RuntimeException(th);
        }
    }

    public static String getString1570(Path path) {
        return new String(getByteArray1569(path), StandardCharsets.UTF_8);
    }
}
