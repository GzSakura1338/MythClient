package me.mioclient.mixin;

import me.mioclient.SpawnTimeHelper_2;
import net.minecraft.entity.EntityType;
import net.minecraft.entity.decoration.EndCrystalEntity;
import net.minecraft.world.World;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({EndCrystalEntity.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinEndCrystalEntity.class */
public class MixinEndCrystalEntity implements SpawnTimeHelper_2 {

    @Unique
    private long spawnTime;

    @Unique
    private boolean mioAttacked;

    @Inject(method = {"<init>(Lnet/minecraft/entity/EntityType;Lnet/minecraft/world/World;)V"}, at = {@At("RETURN")})
    private void init(EntityType<?> entityType, World world, CallbackInfo callbackInfo) {
        this.spawnTime = System.currentTimeMillis();
        this.mioAttacked = false;
    }

    @Override // me.mioclient.SpawnTimeHelper_2
    public long getSpawnTime() {
        return this.spawnTime;
    }

    @Override // me.mioclient.SpawnTimeHelper_2
    public boolean isMioAttacked() {
        return this.mioAttacked;
    }

    @Override // me.mioclient.SpawnTimeHelper_2
    public void setMioAttacked(boolean z) {
        this.mioAttacked = z;
    }
}
