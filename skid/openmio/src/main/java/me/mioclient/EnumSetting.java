package me.mioclient;

import com.google.gson.JsonElement;
import com.google.gson.JsonPrimitive;
import java.lang.Enum;
import me.mioclient.api.Setting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/EnumSetting.class */
public final class EnumSetting<T> extends Setting<T> {
    public final EnumSettingConverter enumSettingConverter;

    /* JADX WARN: Multi-variable type inference failed */
    public EnumSetting(String str, T t, java.util.function.Predicate<T> predicate) {
        super(str, t, predicate);
        this.enumSettingConverter = new EnumSettingConverter(((Enum) getValue()).getDeclaringClass());
        if (!EnumSettingHelper.is1880(t)) {
            throw new IllegalArgumentException();
        }
    }

    /* JADX WARN: Multi-variable type inference failed */
    public EnumSetting(String str, T t) {
        super(str, t);
        this.enumSettingConverter = new EnumSettingConverter(((Enum) getValue()).getDeclaringClass());
        if (!EnumSettingHelper.is1880(t)) {
            throw new IllegalArgumentException();
        }
    }

    @Override // me.mioclient.api.Setting
    public void do134(String str) {
        EnumSetting<T> enumSetting = this;
        enumSetting.do2333((T) this.enumSettingConverter.doBackward(new JsonPrimitive(str)));
    }

    /* JADX WARN: Multi-variable type inference failed */
    @Override // me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        return this.enumSettingConverter.doForward((Enum) getValue());
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        do2333((T) this.enumSettingConverter.doBackward(jsonElement));
    }
}
