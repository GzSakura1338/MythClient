package me.mioclient;

import java.util.Collection;
import java.util.Collections;
import net.minecraft.registry.Registries;
import net.minecraft.registry.RegistryKeys;
import net.minecraft.util.Identifier;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Helper_8.class */
public class Helper_8 implements Helper {
    @Override // me.mioclient.Helper
    public Collection<String> getCollection53(String str) {
        Collection<String> collection53 = Helper.getHelper56(Registries.BLOCK).getCollection53(str);
        return !collection53.isEmpty() ? collection53 : Collections.emptyList();
    }

    @Override // me.mioclient.Helper
    public Collection<String> getCollection54() {
        return Helper.getHelper56(Registries.BLOCK).getCollection54();
    }

    @Override // me.mioclient.Helper
    public Identifier getIdentifier55() {
        return RegistryKeys.ITEM.getValue();
    }
}
