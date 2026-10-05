package me.mioclient;

import java.util.ArrayList;
import net.minecraft.client.MinecraftClient;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Direction;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Runnable.class */
public class Runnable implements java.lang.Runnable {
    public static final BlockPos.Mutable mutable = new BlockPos.Mutable();
    public final HoleSnapSearchHelper4_5 holeSnapSearchHelper4_5;

    public Runnable(HoleSnapSearchHelper4_5 holeSnapSearchHelper4_5) {
        this.holeSnapSearchHelper4_5 = holeSnapSearchHelper4_5;
    }

    @Override // java.lang.Runnable
    public void run() {
        while (true) {
            try {
                do160();
                Thread.sleep(50L);
            } catch (Throwable th) {
            }
        }
    }

    public void do160() {
        if (MinecraftClient.getInstance() == null || MinecraftClient.getInstance().player == null || MinecraftClient.getInstance().world == null) {
            return;
        }
        ArrayList<HoleSnapData> arrayList = new ArrayList<>();
        BlockPos blockPos = MinecraftClient.getInstance().player.getBlockPos();
        for (int i = -16; i < 16; i++) {
            for (int i2 = -16; i2 < 16; i2++) {
                for (int i3 = -16; i3 < 16; i3++) {
                    mutable.set(blockPos.getX() + i, blockPos.getY() + i2, blockPos.getZ() + i3);
                    HoleSnapData holeSnapData2724 = HoleSnapSearchHelper4_5.getHoleSnapData2724(mutable, (Direction) null);
                    if (holeSnapData2724 != null) {
                        if (!arrayList.stream().anyMatch(holeSnapData -> {
                            return holeSnapData2724.getBox799().intersects(holeSnapData.getBox799());
                        })) {
                            arrayList.add(holeSnapData2724);
                        }
                    }
                }
            }
        }
        this.holeSnapSearchHelper4_5.do2727(arrayList);
    }
}
