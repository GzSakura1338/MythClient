package me.mioclient;

import dev.tr7zw.entityculling.versionless.EntityCullingVersionlessBase;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ESPHelper_2.class */
public class ESPHelper_2 implements ESPHelper_3 {
    @Override // me.mioclient.ESPHelper_3
    public boolean is1665() {
        return EntityCullingVersionlessBase.enabled;
    }

    @Override // me.mioclient.ESPHelper_3
    public void do1666(boolean z) {
        EntityCullingVersionlessBase.enabled = z;
    }
}
