package me.mioclient;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.runtime.ObjectMethods;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/MixinPlayerEntityData.class */
public final class MixinPlayerEntityData implements EnumSettingHelper {
    public final String name;
    public final NameTagsHelperMode nameTagsHelperMode;

    public MixinPlayerEntityData(String str, NameTagsHelperMode nameTagsHelperMode) {
        this.name = str;
        this.nameTagsHelperMode = nameTagsHelperMode;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }




    public String getString333() {
        return this.name;
    }

    public NameTagsHelperMode getNameTagsHelperMode631() {
        return this.nameTagsHelperMode;
    }
}
