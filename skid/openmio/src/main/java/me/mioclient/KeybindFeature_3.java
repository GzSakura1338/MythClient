package me.mioclient;

import me.mioclient.api.Keybind;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/KeybindFeature_3.class */
public class KeybindFeature_3 extends KeybindFeature {
    public boolean flag;

    public KeybindFeature_3(String str, Keybind keybind) {
        super(str, Mode_4.HOLD, keybind);
    }

    @Override // me.mioclient.KeybindFeature
    public void run() {
    }

    public void do1072(boolean z) {
        if (this.flag == z) {
            return;
        }
        this.flag = z;
        if (this.list.isEmpty()) {
            return;
        }
        if (this.flag) {
            do2060((String) this.list.getFirst());
        } else {
            do2060((String) this.list.getLast());
        }
    }
}
