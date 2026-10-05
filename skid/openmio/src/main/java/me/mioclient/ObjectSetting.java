package me.mioclient;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import it.unimi.dsi.fastutil.objects.ObjectOpenHashSet;
import java.util.Collection;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import me.mioclient.api.Setting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ObjectSetting.class */
public abstract class ObjectSetting<T> extends Setting<Set<T>> {
    public T object;

    @SafeVarargs
    public ObjectSetting(String str, T... tArr) {
        super(str, new ObjectOpenHashSet());
        ((Set) getObject2324()).addAll(List.of((Object[]) tArr));
        ((Set) getValue()).addAll(List.of((Object[]) tArr));
    }

    @SafeVarargs
    public ObjectSetting(String str, java.util.function.Predicate<Set<T>> predicate, T... tArr) {
        super(str, new ObjectOpenHashSet(), predicate);
        ((Set) getObject2324()).addAll(List.of((Object[]) tArr));
        ((Set) getValue()).addAll(List.of((Object[]) tArr));
    }

    public abstract T getObject1070(String str);

    public abstract String getString1069(T t);

    public abstract Collection<T> getCollection1068();

    /* JADX WARN: Multi-variable type inference failed */
    public Set<String> getSet3126() {
        Set objectOpenHashSet = new ObjectOpenHashSet();
        Iterator it = ((Set) getValue()).iterator();
        while (it.hasNext()) {
            objectOpenHashSet.add(getString1069((T) it.next()));
        }
        return objectOpenHashSet;
    }

    @Override // me.mioclient.api.Setting
    public void do134(String str) {
        T object1070 = getObject1070(str);
        if (object1070 == null) {
            throw new IllegalArgumentException();
        }
        if (is3132(object1070)) {
            do3130(object1070);
        } else {
            do3128(object1070);
        }
    }

    public Collection<String> getCollection3127() {
        HashSet hashSet = new HashSet();
        Iterator<T> it = getCollection1068().iterator();
        while (it.hasNext()) {
            hashSet.add(getString1069(it.next()));
        }
        return hashSet;
    }

    public void do3128(T t) {
        if (t == null) {
            return;
        }
        ((Set) getValue()).add(t);
        this.object = t;
        if (this.runnable != null) {
            this.runnable.run();
        }
    }

    public void do3129(String str) {
        if (str == null) {
            return;
        }
        do3128(getObject1070(str));
    }

    public void do3130(T t) {
        if (t == null) {
            return;
        }
        ((Set) getValue()).remove(t);
        this.object = t;
        if (this.runnable != null) {
            this.runnable.run();
        }
    }

    public void do3131(String str) {
        if (str == null) {
            return;
        }
        do3130(getObject1070(str));
    }

    public boolean is3132(T t) {
        if (t == null) {
            return false;
        }
        return ((Set) getValue()).contains(t);
    }

    public boolean is3133(String str) {
        if (str == null) {
            return false;
        }
        return is3132(getObject1070(str));
    }

    public T getObject3134() {
        return this.object;
    }

    /* JADX WARN: Multi-variable type inference failed */
    @Override // me.mioclient.PresetHelper_7
    public JsonElement toJson() {
        JsonArray jsonArray = new JsonArray();
        Iterator it = ((Set) getValue()).iterator();
        while (it.hasNext()) {
            jsonArray.add(getString1069((T) it.next()));
        }
        return jsonArray;
    }
}
