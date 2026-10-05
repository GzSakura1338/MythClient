/*
 * Decompiled with CFR 0.2.2 (FabricMC 7c48b8c4).
 */
package nick;

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.InputStream;
import java.lang.reflect.Field;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;
import sun.misc.Unsafe;

public final class Strings {
    private static Unsafe UNSAFE;
    private static long O1;
    private static long O2;
    private static Map<Integer, String> STRINGS;

    public static String decode(byte[] data) {
        try {
            String str = (String)UNSAFE.allocateInstance(String.class);
            byte coder = data[0];
            byte[] v = new byte[data.length - 1];
            System.arraycopy(data, 1, v, 0, v.length);
            UNSAFE.putByteVolatile(str, O1, coder);
            UNSAFE.putObjectVolatile(str, O2, v);
            int __ = str.hashCode();
            return str;
        } catch (Throwable _t) {
            _t.printStackTrace(System.err);
            return null;
        }
    }

    public static byte[] encode(String str) {
        byte[] v = (byte[])UNSAFE.getObjectVolatile(str, O2);
        byte coder = UNSAFE.getByteVolatile(str, O1);
        byte[] r = new byte[v.length + 1];
        r[0] = coder;
        System.arraycopy(v, 0, r, 1, v.length);
        return r;
    }

    public static Map<Integer, String> parse(byte[] data) {
        try {
            DataInputStream dis = new DataInputStream(new ByteArrayInputStream(data));
            int size = dis.readInt();
            HashMap<Integer, String> m = new HashMap<Integer, String>(size);
            for (int i = 0; i < size; ++i) {
                int type = dis.read();
                int key = dis.readInt();
                int len = dis.readInt();
                byte[] str = new byte[len];
                dis.readFully(str);
                if (type == 0) {
                    m.put(key, Strings.decode(str));
                    continue;
                }
                if (type == 1) {
                    m.put(key, new String(str, StandardCharsets.UTF_8));
                    continue;
                }
                throw new RuntimeException(String.valueOf(type));
            }
            if (dis.available() > 0) {
                throw new RuntimeException(new String(dis.readAllBytes()));
            }
            dis.close();
            return m;
        } catch (Throwable _t) {
            _t.printStackTrace(System.err);
            return null;
        }
    }

    public static String getString(int key) {
        return Objects.requireNonNull(STRINGS.getOrDefault(key, null));
    }

    static {
        try {
            Field f = Unsafe.class.getDeclaredField("theUnsafe");
            f.setAccessible(true);
            UNSAFE = (Unsafe)f.get(null);
            O1 = UNSAFE.objectFieldOffset(String.class.getDeclaredField("coder"));
            O2 = UNSAFE.objectFieldOffset(String.class.getDeclaredField("value"));
            try (InputStream is = Strings.class.getClassLoader().getResourceAsStream("strings.bin");){
                STRINGS = Strings.parse(Objects.requireNonNull(is).readAllBytes());
            }
        } catch (Throwable _t) {
            _t.printStackTrace(System.err);
        }
    }
}

