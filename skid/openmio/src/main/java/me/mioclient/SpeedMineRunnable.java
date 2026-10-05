package me.mioclient;

import me.mioclient.module.player.SpeedMine;
import net.minecraft.item.Item;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SpeedMineRunnable.class */
public class SpeedMineRunnable implements java.lang.Runnable, SearchHelper_4 {
    public final SpeedMine speedMine;
    public final long num = System.currentTimeMillis();
    public final Item item;
    public final int num2;
    public final int num3;

    public SpeedMineRunnable(SpeedMine speedMine, Item item, int i, int i2) {
        this.item = item;
        this.num2 = i;
        this.num3 = i2;
        this.speedMine = speedMine;
    }

    @Override // java.lang.Runnable
    public void run() {
        if (this.speedMine.getSpeedMineRunnable1057() == null || this.speedMine.getSpeedMineRunnable1057().equals(this)) {
            if (minecraftClient.player.getInventory().getStack(this.num3).isOf(this.item)) {
                return;
            }
            FireworksHelper.do439(this.num2);
        }
    }

    public boolean equals(Object obj) {
        return (obj instanceof SpeedMineRunnable) && ((SpeedMineRunnable) obj).num == this.num;
    }
}
