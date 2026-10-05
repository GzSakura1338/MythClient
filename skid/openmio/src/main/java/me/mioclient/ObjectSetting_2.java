package me.mioclient;

import com.google.gson.JsonElement;
import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.Locale;
import java.util.Set;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ObjectSetting_2.class */
public final class ObjectSetting_2 extends ObjectSetting<String> {
    public ObjectSetting_2(String str, String... strArr) {
        super(str, strArr);
    }

    public ObjectSetting_2(String str, java.util.function.Predicate<Set<String>> predicate, String... strArr) {
        super(str, predicate, strArr);
    }

    @Override // me.mioclient.ObjectSetting
    /* renamed from: getString1066, reason: merged with bridge method [inline-methods] */
    public String getObject1070(String str) {
        return str.toLowerCase(Locale.ROOT);
    }

    @Override // me.mioclient.ObjectSetting
    /* renamed from: getString1067, reason: merged with bridge method [inline-methods] */
    public String getString1069(String str) {
        return str.toLowerCase(Locale.ROOT);
    }

    @Override // me.mioclient.ObjectSetting
    public Collection<String> getCollection1068() {
        return Collections.emptyList();
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        if (jsonElement.isJsonArray()) {
            getValue().clear();
            Iterator it = jsonElement.getAsJsonArray().iterator();
            while (it.hasNext()) {
                getValue().add(((JsonElement) it.next()).getAsString());
            }
        }
    }
}
