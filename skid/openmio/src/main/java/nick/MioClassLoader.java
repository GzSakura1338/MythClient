/*
 * Decompiled with CFR 0.2.2 (FabricMC 7c48b8c4).
 */
package nick;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.security.SecureClassLoader;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;
import nick.Hash;

public final class MioClassLoader
extends SecureClassLoader {
    private static final Map<Long, byte[]> DATA = new HashMap<Long, byte[]>();
    private final ClassLoader loader;

    private static byte[] read(String path) {
        try (java.io.InputStream is = MioClassLoader.class.getClassLoader().getResourceAsStream(path)) {
            return is == null ? null : is.readAllBytes();
        } catch (Throwable _t) { return null; }
    }

    public MioClassLoader(ClassLoader l1, ClassLoader l2) {
        super(l1);
        this.loader = l2;
    }

    @Override
    public Class<?> loadClass(String klass) throws ClassNotFoundException {
        return this.loader.loadClass(klass);
    }

    @Override
    protected Class<?> findClass(String string) throws ClassNotFoundException {
        return super.findClass(string);
    }

    @Override
    public URL getResource(String string) {
        return this.loader.getResource(string);
    }

    @Override
    public InputStream getResourceAsStream(String string) {
        InputStream stream = this.loader.getResourceAsStream(string);
        if (stream != null) {
            return stream;
        }
        long hash = Hash.hash(string.getBytes(StandardCharsets.UTF_8));
        byte[] data = DATA.getOrDefault(hash, null);
        if (data == null) {
            return null;
        }
        return new ByteArrayInputStream(data);
    }

    static {
        DATA.put(-202295177518396515L, MioClassLoader.read("mio/-202295177518396515.bin"));
        DATA.put(-2236490846758828587L, MioClassLoader.read("mio/-2236490846758828587.bin"));
        DATA.put(-2587742845299202020L, MioClassLoader.read("mio/-2587742845299202020.bin"));
        DATA.put(-2721754409736099035L, MioClassLoader.read("mio/-2721754409736099035.bin"));
        DATA.put(-3566755497116400425L, MioClassLoader.read("mio/-3566755497116400425.bin"));
        DATA.put(-4045037417574934788L, MioClassLoader.read("mio/-4045037417574934788.bin"));
        DATA.put(-5625967054364314949L, MioClassLoader.read("mio/-5625967054364314949.bin"));
        DATA.put(-6862787127220354815L, MioClassLoader.read("mio/-6862787127220354815.bin"));
        DATA.put(-7023810658846360874L, MioClassLoader.read("mio/-7023810658846360874.bin"));
        DATA.put(-7183123058599273620L, MioClassLoader.read("mio/-7183123058599273620.bin"));
        DATA.put(-7341433280440695994L, MioClassLoader.read("mio/-7341433280440695994.bin"));
        DATA.put(-7680579496922721047L, MioClassLoader.read("mio/-7680579496922721047.bin"));
        DATA.put(-8099616702159452490L, MioClassLoader.read("mio/-8099616702159452490.bin"));
        DATA.put(-8381037589029380890L, MioClassLoader.read("mio/-8381037589029380890.bin"));
        DATA.put(-8794909434937775667L, MioClassLoader.read("mio/-8794909434937775667.bin"));
        DATA.put(1539974198544823836L, MioClassLoader.read("mio/1539974198544823836.bin"));
        DATA.put(1663163836379652005L, MioClassLoader.read("mio/1663163836379652005.bin"));
        DATA.put(1790238393984445022L, MioClassLoader.read("mio/1790238393984445022.bin"));
        DATA.put(1896199715147295766L, MioClassLoader.read("mio/1896199715147295766.bin"));
        DATA.put(2068539170343643742L, MioClassLoader.read("mio/2068539170343643742.bin"));
        DATA.put(2138326405829089144L, MioClassLoader.read("mio/2138326405829089144.bin"));
        DATA.put(217053583388222802L, MioClassLoader.read("mio/217053583388222802.bin"));
        DATA.put(2772677987641866972L, MioClassLoader.read("mio/2772677987641866972.bin"));
        DATA.put(340059997659842985L, MioClassLoader.read("mio/340059997659842985.bin"));
        DATA.put(3744080819900926215L, MioClassLoader.read("mio/3744080819900926215.bin"));
        DATA.put(450073944724737926L, MioClassLoader.read("mio/450073944724737926.bin"));
        DATA.put(5683599352333742209L, MioClassLoader.read("mio/5683599352333742209.bin"));
        DATA.put(5685221327652941277L, MioClassLoader.read("mio/5685221327652941277.bin"));
        DATA.put(6356538713265320868L, MioClassLoader.read("mio/6356538713265320868.bin"));
        DATA.put(6362189062087683333L, MioClassLoader.read("mio/6362189062087683333.bin"));
        DATA.put(6385281682528407296L, MioClassLoader.read("mio/6385281682528407296.bin"));
        DATA.put(6735045970717493054L, MioClassLoader.read("mio/6735045970717493054.bin"));
        DATA.put(719574448491822624L, MioClassLoader.read("mio/719574448491822624.bin"));
        DATA.put(7213105665262162251L, MioClassLoader.read("mio/7213105665262162251.bin"));
        DATA.put(7486200784412077737L, MioClassLoader.read("mio/7486200784412077737.bin"));
        DATA.put(7532973076254138070L, MioClassLoader.read("mio/7532973076254138070.bin"));
        DATA.put(7949560956520799428L, MioClassLoader.read("mio/7949560956520799428.bin"));
        DATA.put(8325382314658498605L, MioClassLoader.read("mio/8325382314658498605.bin"));
        DATA.put(8480236269238451330L, MioClassLoader.read("mio/8480236269238451330.bin"));
        DATA.put(8810857770749952228L, MioClassLoader.read("mio/8810857770749952228.bin"));
        DATA.put(8818174620704159780L, MioClassLoader.read("mio/8818174620704159780.bin"));
        DATA.put(9098684348202532870L, MioClassLoader.read("mio/9098684348202532870.bin"));
    }
}

