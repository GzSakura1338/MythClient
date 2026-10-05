package me.mioclient;

import me.mioclient.api.Setting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/HoleSnapMode.class */
public enum HoleSnapMode {
    MIN { // from class: me.mioclient.HoleSnapMode.Inner
        @Override // me.mioclient.HoleSnapMode
        public boolean is220(Setting<?> setting) {
            return setting.getValue().equals(setting.getObject2325());
        }
    },
    MAX { // from class: me.mioclient.HoleSnapMode.Inner_2
        @Override // me.mioclient.HoleSnapMode
        public boolean is220(Setting<?> setting) {
            return setting.getValue().equals(setting.getObject2326());
        }
    };

    public boolean is220(Setting<?> setting) {
        return false;
    }
}
