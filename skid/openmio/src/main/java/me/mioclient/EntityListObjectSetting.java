package me.mioclient;

import com.google.gson.JsonElement;
import java.util.Collection;
import java.util.Iterator;
import java.util.Set;
import net.minecraft.registry.Registry;
import net.minecraft.util.Identifier;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/EntityListObjectSetting.class */
public class EntityListObjectSetting<T> extends ObjectSetting<T> {
    public final Registry<T> registry;

    @SafeVarargs
    public EntityListObjectSetting(String str, Registry<T> registry, T... tArr) {
        super(str, tArr);
        this.registry = registry;
    }

    @SafeVarargs
    public EntityListObjectSetting(String str, Registry<T> registry, java.util.function.Predicate<Set<T>> predicate, T... tArr) {
        super(str, predicate, tArr);
        this.registry = registry;
    }

    @Override // me.mioclient.ObjectSetting
    public T getObject1070(String str) {
        return (T) this.registry.getOrEmpty(Identifier.of(str)).orElse(null);
    }

    @Override // me.mioclient.ObjectSetting
    public String getString1069(T t) {
        Identifier id = this.registry.getId(t);
        if (id == null) {
            return null;
        }
        return id.toShortTranslationKey();
    }

    @Override // me.mioclient.ObjectSetting
    public Collection<T> getCollection1068() {
        return this.registry.stream().toList();
    }

    @Override // me.mioclient.PresetHelper_7
    public void fromJson(JsonElement jsonElement) {
        if (jsonElement.isJsonArray()) {
            ((Set) getValue()).clear();
            Iterator it = jsonElement.getAsJsonArray().iterator();
            while (it.hasNext()) {
                JsonElement jsonElement2 = (JsonElement) it.next();
                ((Set) getValue()).add(this.registry.get(Identifier.of(jsonElement2.getAsString())));
            }
        }
    }

    public Registry<T> getRegistry2387() {
        return this.registry;
    }
}
