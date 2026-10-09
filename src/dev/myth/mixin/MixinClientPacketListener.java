package dev.myth.mixin;

import com.llamalad7.mixinextras.injector.wrapmethod.WrapMethod;
import com.llamalad7.mixinextras.injector.wrapoperation.Operation;
import com.llamalad7.mixinextras.injector.wrapoperation.WrapOperation;
import dev.myth.Myth;
import dev.myth.event.api.EventType;
import dev.myth.event.impl.ChatEvent;
import dev.myth.event.impl.PacketEvent;
import dev.myth.model.render.NoRender;
import net.minecraft.client.multiplayer.ClientPacketListener;
import net.minecraft.network.PacketListener;
import net.minecraft.network.protocol.Packet;
import net.minecraft.network.protocol.game.ClientboundAddEntityPacket;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(ClientPacketListener.class)
public abstract class MixinClientPacketListener {
    @WrapMethod(method = "sendChat")
    private void myth$sendChat(String message, Operation<Void> original) {
        ChatEvent event = Myth.EVENT_MANAGER.call(new ChatEvent(message));
        if (!event.isCancelled()) {
            original.call(event.getMessage());
        }
    }

    /**
     * Bundled packets are unwrapped by handleBundlePacket and dispatched straight to their handlers,
     * so they never reach Connection.channelRead0 and never raised a PacketEvent. ServerEntity
     * sends every entity spawn inside a bundle, so without this hook a listener cannot see them.
     * Unlike the Connection hook this one runs on the client thread.
     */
    @WrapOperation(method = "handleBundlePacket", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/network/protocol/Packet;handle(Lnet/minecraft/network/PacketListener;)V"))
    private void myth$bundledPacket(Packet<?> subPacket, PacketListener listener, Operation<Void> original) {
        PacketEvent event = Myth.EVENT_MANAGER.call(new PacketEvent(EventType.INCOMING, subPacket));
        if (!event.isCancelled()) original.call(event.getPacket(), listener);
    }

    /** Mio Removal = Full: the filtered entity never enters the world, not just the render pass. */
    @Inject(method = "handleAddEntity", at = @At("HEAD"), cancellable = true)
    private void myth$dropEntitySpawn(ClientboundAddEntityPacket packet, CallbackInfo ci) {
        NoRender noRender = NoRender.active();
        if (noRender != null && noRender.dropsEntitySpawn() && noRender.hidesEntity(packet.getType())) ci.cancel();
    }
}
