package me.mioclient.mixin.ducks;

import net.minecraft.client.network.PendingUpdateManager;
import net.minecraft.client.world.ClientWorld;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({ClientWorld.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckClientWorld.class */
public interface DuckClientWorld {
    @Accessor("pendingUpdateManager")
    PendingUpdateManager getPendingUpdateManager();
}
