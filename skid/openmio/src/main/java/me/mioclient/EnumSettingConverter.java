package me.mioclient;

import com.google.gson.JsonElement;
import com.google.gson.JsonPrimitive;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/EnumSettingConverter.class */
public class EnumSettingConverter extends com.google.common.base.Converter<Enum, JsonElement> {
    public final Class<? extends Enum> class_;

    public EnumSettingConverter(Class<? extends Enum<?>> cls) {
        this.class_ = cls;
    }

    public static int get911(Enum<?> r3) {
        for (int i = 0; i < ((Enum[]) r3.getDeclaringClass().getEnumConstants()).length; i++) {
            Enum r0 = ((Enum[]) r3.getDeclaringClass().getEnumConstants())[i];
            if (!EnumSettingConverterHelper.is1630(r0)) {
                if (r0.name().equalsIgnoreCase(r3.name())) {
                    return i;
                }
            }
        }
        return -1;
    }

    public static Enum<?> getEnum912(Enum r3) {
        int i = get911(r3);
        for (int i2 = 0; i2 < r3.getDeclaringClass().getEnumConstants().length; i2++) {
            Enum<?> r0 = (Enum) r3.getDeclaringClass().getEnumConstants()[i2];
            if (!EnumSettingConverterHelper.is1630(r0) && i2 > i) {
                return r0;
            }
        }
        return (Enum) r3.getDeclaringClass().getEnumConstants()[0];
    }

    /* JADX WARN: Multi-variable type inference failed */
    public static String getString913(Enum<?> r3) {
        if (r3 instanceof EnumSettingHelper) {
            return ((EnumSettingHelper) r3).getName();
        }
        throw new IllegalArgumentException();
    }

    /* JADX WARN: Multi-variable type inference failed */
    /* renamed from: getJsonElement914, reason: merged with bridge method [inline-methods] */
    public JsonElement doForward(Enum r5) {
        if (r5 instanceof EnumSettingHelper) {
            return new JsonPrimitive(((EnumSettingHelper) r5).getName());
        }
        throw new IllegalArgumentException();
    }

    /* JADX WARN: Multi-variable type inference failed */
    /* renamed from: getEnum915, reason: merged with bridge method [inline-methods] */
    public Enum doBackward(JsonElement jsonElement) {
        for (Enum named : (Enum[]) this.class_.getEnumConstants()) {
            if (!EnumSettingHelper.is1880(named)) {
                throw new IllegalArgumentException();
            }
            if (!EnumSettingConverterHelper.is1630(named) && (named instanceof EnumSettingHelper)) {
                if (jsonElement.getAsString().equalsIgnoreCase(((EnumSettingHelper) named).getName())) {
                    return named;
                }
            }
        }
        return ((Enum[]) this.class_.getEnumConstants())[0];
    }
}
