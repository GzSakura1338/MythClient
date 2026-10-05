package me.mioclient;

import java.util.Collection;
import me.mioclient.api.Setting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ScaffoldMode_2.class */
public enum ScaffoldMode_2 implements EnumSettingHelper {
    ANY("Any"),
    WHITELIST("WhiteList") { // from class: me.mioclient.ScaffoldMode_2.Inner
        @Override // me.mioclient.ScaffoldMode_2
        public <T> boolean is1391(T t, Collection<T> collection) {
            return collection.contains(t);
        }
    },
    BLACKLIST("BlackList") { // from class: me.mioclient.ScaffoldMode_2.Inner_2
        @Override // me.mioclient.ScaffoldMode_2
        public <T> boolean is1391(T t, Collection<T> collection) {
            return !WHITELIST.is1391(t, collection);
        }
    };

    public final String name;

    ScaffoldMode_2(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }

    public <T> boolean is1391(T t, Collection<T> collection) {
        return true;
    }

    public <T> boolean is1392(T t, Setting<? extends Collection<T>> setting) {
        return is1391(t, setting.getValue());
    }
}
