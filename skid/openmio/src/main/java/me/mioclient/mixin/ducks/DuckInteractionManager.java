package me.mioclient.mixin.ducks;

import net.minecraft.client.network.ClientPlayerInteractionManager;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Invoker;

/* compiled from: 0.java */
@Mixin({ClientPlayerInteractionManager.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckInteractionManager.class */
public interface DuckInteractionManager {
    @Invoker("syncSelectedSlot")
    void sync();
}
