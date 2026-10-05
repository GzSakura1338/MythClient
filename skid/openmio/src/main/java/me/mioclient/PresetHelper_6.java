package me.mioclient;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Path;
import java.util.HashMap;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/PresetHelper_6.class */
public final class PresetHelper_6 {
    public final HashMap<String, String> hashMap = new HashMap<>();

    public PresetHelper_6() {
        this.hashMap.put("textures/overlay.png", "assets/mio/textures/overlay.png");
        this.hashMap.put("textures/skin_protect.png", "assets/mio/textures/skin_protect.png");
        this.hashMap.put("textures/shine.png", "assets/mio/textures/shine.png");
    }

    public void do72() {
        this.hashMap.forEach(this::do2114);
    }

    public void do2114(String str, String str2) {
        Path resolve = PresetHelper.path.resolve(str);
        if (resolve.toFile().exists()) {
            return;
        }
        try {
            InputStream resourceAsStream = Helper_16.class.getClassLoader().getResourceAsStream(str2);
            if (resourceAsStream == null) {
                if (resourceAsStream != null) {
                    resourceAsStream.close();
                }
            } else {
                try {
                    PresetHelper_4.do1568(resolve, resourceAsStream.readAllBytes());
                    if (resourceAsStream != null) {
                        resourceAsStream.close();
                    }
                } finally {
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
