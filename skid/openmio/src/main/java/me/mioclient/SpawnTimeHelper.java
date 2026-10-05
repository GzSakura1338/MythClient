package me.mioclient;

import net.minecraft.util.Formatting;
import net.minecraft.util.math.BlockPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SpawnTimeHelper.class */
public final class SpawnTimeHelper {
    public transient long spawnTime = System.currentTimeMillis();
    public final String string;
    public final String string2;
    public final int x;
    public final int y;
    public final int num;
    public transient BlockPos blockPos;

    public SpawnTimeHelper(String str, String str2, int i, int i2, int i3) {
        this.string = str;
        this.string2 = str2;
        this.x = i;
        this.y = i2;
        this.num = i3;
    }

    public SpawnTimeHelper(String str, String str2, BlockPos blockPos) {
        this.string = str;
        this.string2 = str2;
        this.x = blockPos.getX();
        this.y = blockPos.getY();
        this.num = blockPos.getZ();
    }

    public String getString793() {
        return this.string;
    }

    public String getString794() {
        return Formatting.strip(this.string);
    }

    public String getString518() {
        return this.string2;
    }

    public int getX() {
        return this.x;
    }

    public int getY() {
        return this.y;
    }

    public int get795() {
        return this.num;
    }

    public BlockPos getBlockPos386() {
        if (this.blockPos == null) {
            this.blockPos = new BlockPos(this.x, this.y, this.num);
        }
        return this.blockPos;
    }

    public long getSpawnTime() {
        return this.spawnTime;
    }

    public void reset() {
        this.spawnTime = System.currentTimeMillis();
    }

    public boolean is796() {
        String str = null;
        try {
            str = SearchHelper_4.minecraftClient.getNetworkHandler().getConnection().getAddress().toString();
            int indexOf = str.indexOf(47);
            if (indexOf > 0) {
                str = str.substring(0, indexOf);
            }
            while (str.endsWith(".")) {
                str = str.substring(0, str.length() - 1);
            }
        } catch (Exception e) {
        }
        if (str == null || str.isEmpty()) {
            str = "singleplayer";
        }
        return str.equals(this.string2);
    }
}
