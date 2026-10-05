package me.mioclient.mixin.ducks;

import io.netty.channel.Channel;
import net.minecraft.network.ClientConnection;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({ClientConnection.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckClientConnection.class */
public interface DuckClientConnection {
    @Accessor("channel")
    Channel getChannel();
}
