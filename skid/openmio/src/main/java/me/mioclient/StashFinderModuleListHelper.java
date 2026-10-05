package me.mioclient;

import java.util.ArrayList;
import java.util.List;
import me.mioclient.EnumSettingHelper;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/StashFinderModuleListHelper.class */
public class StashFinderModuleListHelper<E extends EnumSettingHelper> extends ModuleListHelper<E, List<E>> {
    public StashFinderModuleListHelper() {
        super(new ArrayList());
    }

    /* JADX WARN: Incorrect return type in method signature: <T:TE;>(Ljava/lang/Class<TT;>;)TT; */
    public EnumSettingHelper getEnumSettingHelper120(Class cls) {
        return (EnumSettingHelper) getObject2405(enumSettingHelper -> {
            return enumSettingHelper.getClass().equals(cls);
        });
    }

    @Override // me.mioclient.Helper_9
    /* renamed from: is119, reason: merged with bridge method [inline-methods] */
    public boolean register(E e) {
        if (e == null) {
            return false;
        }
        for (EnumSettingHelper enumSettingHelper : (List<E>) this.registry) {
            if (enumSettingHelper != null && enumSettingHelper.getName() != null && enumSettingHelper.getName().equals(e.getName())) {
                return false;
            }
        }
        ((List) this.registry).add(e);
        return true;
    }

    @Override // me.mioclient.Helper_9
    /* renamed from: is118, reason: merged with bridge method [inline-methods] */
    public boolean unregister(E e) {
        if (e == null) {
            return false;
        }
        ((List) this.registry).remove(e);
        return true;
    }
}
