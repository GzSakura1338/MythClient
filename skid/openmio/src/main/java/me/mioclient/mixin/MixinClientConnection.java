package me.mioclient.mixin;

import io.netty.channel.Channel;
import io.netty.channel.ChannelHandlerContext;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.MixinMessageIndicatorHelper;
import me.mioclient.SearchHelper_4;
import me.mioclient.event.ChannelRead0Event;
import me.mioclient.event.SendImmediatelyEvent;
import me.mioclient.event.SendInternalEvent;
import me.mioclient.module.misc.NoPacketKick;
import net.minecraft.network.ClientConnection;
import net.minecraft.network.NetworkSide;
import net.minecraft.network.PacketCallbacks;
import net.minecraft.network.packet.Packet;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({ClientConnection.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinClientConnection.class */
public class MixinClientConnection {
    private static NoPacketKick nopacketkick = (NoPacketKick) BaritoneHelper_3.baritoneHelper_4.getModule117(NoPacketKick.class);

    @Shadow
    private Channel field_11651;

    @Shadow
    @Final
    private NetworkSide field_11643;

    @Inject(method = {"channelRead0*"}, at = {@At("HEAD")}, cancellable = true)
    public void channelRead0(ChannelHandlerContext channelHandlerContext, Packet<?> packet, CallbackInfo callbackInfo) {
        if (!this.field_11651.isOpen() || packet == null) {
            return;
        }
        try {
            ChannelRead0Event channelRead0Event = new ChannelRead0Event(packet);
            SearchHelper_4.baritoneHelper.getObject1794(channelRead0Event);
            if (channelRead0Event.is2403()) {
                callbackInfo.cancel();
            }
        } catch (Exception e) {
        }
    }

    @Inject(method = {"sendImmediately"}, at = {@At("HEAD")}, cancellable = true)
    private void sendImmediately(Packet<?> packet, PacketCallbacks packetCallbacks, boolean z, CallbackInfo callbackInfo) {
        if (this.field_11643 != NetworkSide.CLIENTBOUND) {
            return;
        }
        try {
            SendImmediatelyEvent sendImmediatelyEvent = new SendImmediatelyEvent(packet);
            SearchHelper_4.baritoneHelper.getObject1794(sendImmediatelyEvent);
            if (sendImmediatelyEvent.is2403()) {
                callbackInfo.cancel();
            }
        } catch (Exception e) {
        }
    }

    @Inject(method = {"sendInternal"}, at = {@At("TAIL")})
    private void sendInternal(Packet<?> packet, PacketCallbacks packetCallbacks, boolean z, CallbackInfo callbackInfo) {
        if (this.field_11643 != NetworkSide.CLIENTBOUND) {
            return;
        }
        try {
            SearchHelper_4.baritoneHelper.getObject1794(new SendInternalEvent(packet));
        } catch (Exception e) {
        }
    }

    @Inject(method = {"exceptionCaught"}, at = {@At("HEAD")}, cancellable = true)
    private void exceptionCaught(ChannelHandlerContext channelHandlerContext, Throwable th, CallbackInfo callbackInfo) {
        if (nopacketkick.isToggled()) {
            if (nopacketkick.message.getValue().booleanValue()) {
                try {
                    MixinMessageIndicatorHelper.do345(nopacketkick.getText1411(th), MixinMessageIndicatorHelper.getMessageSignatureData339(nopacketkick), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode);
                } catch (Throwable th2) {
                }
            }
            callbackInfo.cancel();
        }
    }
}
