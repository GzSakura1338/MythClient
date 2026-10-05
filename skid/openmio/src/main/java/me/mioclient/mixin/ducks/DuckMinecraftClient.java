package me.mioclient.mixin.ducks;

import net.minecraft.client.MinecraftClient;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;
import org.spongepowered.asm.mixin.gen.Invoker;

/* compiled from: 0.java */
@Mixin({MinecraftClient.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckMinecraftClient.class */
public interface DuckMinecraftClient {
    @Invoker("doAttack")
    boolean attack();

    @Invoker("doItemUse")
    void interact();

    @Accessor("itemUseCooldown")
    void setItemUseCooldown(int i);

    @Accessor("itemUseCooldown")
    int getItemUseCooldown();

    @Accessor("disconnecting")
    boolean mio$isDisconnecting();
}
