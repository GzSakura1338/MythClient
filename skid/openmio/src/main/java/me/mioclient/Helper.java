package me.mioclient;

import java.util.Collection;
import java.util.List;
import net.minecraft.registry.Registry;
import net.minecraft.util.Identifier;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Helper.class */
public interface Helper {
    public static final List<Helper> list = List.of(new Helper_12(), new Helper_8());
    public static final Helper_24 helper_24 = new Helper_24();

    Collection<String> getCollection53(String str);

    Collection<String> getCollection54();

    Identifier getIdentifier55();

    static Helper getHelper56(Registry<?> registry) {
        for (Helper helper : list) {
            if (registry.getKey().getValue().equals(helper.getIdentifier55())) {
                return helper;
            }
        }
        return helper_24;
    }
}
