package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ScaffoldMode_3.class */
public enum ScaffoldMode_3 implements SearchHelper_4, EnumSettingHelper {
    NONE("None"),
    NORMAL("Normal") { // from class: me.mioclient.ScaffoldMode_3.Inner_2
        @Override // me.mioclient.ScaffoldMode_3
        public void do2742(int i, java.lang.Runnable runnable) {
            FireworksHelper.do456(i);
            runnable.run();
        }
    },
    SILENT("Silent") { // from class: me.mioclient.ScaffoldMode_3.Inner
        @Override // me.mioclient.ScaffoldMode_3
        public void do2742(int i, java.lang.Runnable runnable) {
            int i2 = minecraftClient.player.getInventory().selectedSlot;
            FireworksHelper.do456(i);
            runnable.run();
            FireworksHelper.do456(i2);
        }
    };

    public final String name;

    ScaffoldMode_3(String str) {
        this.name = str;
    }

    @Override // me.mioclient.EnumSettingHelper
    public String getName() {
        return this.name;
    }

    public void do2742(int i, java.lang.Runnable runnable) {
        if (minecraftClient.player.getInventory().selectedSlot != i) {
            return;
        }
        runnable.run();
    }
}
