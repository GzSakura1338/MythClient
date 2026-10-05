package dev.myth.mixin;

import com.llamalad7.mixinextras.injector.wrapmethod.WrapMethod;
import com.llamalad7.mixinextras.injector.wrapoperation.Operation;
import com.llamalad7.mixinextras.injector.wrapoperation.WrapOperation;
import dev.myth.Myth;
import dev.myth.event.api.EventType;
import dev.myth.event.impl.PacketEvent;
import io.netty.channel.ChannelFutureListener;
import net.minecraft.network.Connection;
import net.minecraft.network.PacketListener;
import net.minecraft.network.protocol.Packet;
import net.minecraft.network.protocol.PacketFlow;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;

@Mixin(Connection.class)
public abstract class MixinConnection {
    @Shadow public abstract PacketFlow getReceiving();

    @WrapOperation(method = "channelRead0(Lio/netty/channel/ChannelHandlerContext;Lnet/minecraft/network/protocol/Packet;)V",
            at = @At(value = "INVOKE", target = "Lnet/minecraft/network/Connection;genericsFtw(Lnet/minecraft/network/protocol/Packet;Lnet/minecraft/network/PacketListener;)V"))
    private void myth$receive(Packet<?> packet, PacketListener listener, Operation<Void> original) {
        // Integrated servers also use Connection; only publish the client's inbound traffic.
        if (getReceiving() != PacketFlow.CLIENTBOUND) {
            original.call(packet, listener);
            return;
        }
        PacketEvent event = Myth.EVENT_MANAGER.call(new PacketEvent(EventType.INCOMING, packet));
        if (!event.isCancelled()) {
            original.call(event.getPacket(), listener);
        }
    }

    @WrapMethod(method = "send(Lnet/minecraft/network/protocol/Packet;Lio/netty/channel/ChannelFutureListener;Z)V")
    private void myth$send(Packet<?> packet, ChannelFutureListener callback, boolean flush, Operation<Void> original) {
        if (getReceiving() != PacketFlow.CLIENTBOUND) {
            original.call(packet, callback, flush);
            return;
        }
        PacketEvent event = Myth.EVENT_MANAGER.call(new PacketEvent(EventType.OUTGOING, packet));
        if (!event.isCancelled()) {
            original.call(event.getPacket(), callback, flush);
        }
    }
}
