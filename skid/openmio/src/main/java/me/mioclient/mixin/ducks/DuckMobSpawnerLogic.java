package me.mioclient.mixin.ducks;

import net.minecraft.block.spawner.MobSpawnerLogic;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({MobSpawnerLogic.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckMobSpawnerLogic.class */
public interface DuckMobSpawnerLogic {
    @Accessor("spawnDelay")
    int getSpawnDelay();
}
