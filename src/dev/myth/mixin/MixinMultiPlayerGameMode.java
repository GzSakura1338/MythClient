package dev.myth.mixin;

import com.llamalad7.mixinextras.injector.wrapoperation.Operation;
import com.llamalad7.mixinextras.injector.wrapoperation.WrapOperation;
import dev.myth.Myth;
import dev.myth.event.impl.PositionItemEvent;
import net.minecraft.client.multiplayer.ClientPacketListener;
import net.minecraft.client.multiplayer.MultiPlayerGameMode;
import net.minecraft.network.protocol.Packet;
import net.minecraft.network.protocol.game.ServerboundUseItemPacket;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;

@Mixin(MultiPlayerGameMode.class)
public abstract class MixinMultiPlayerGameMode {
    @WrapOperation(method = "startPrediction", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/client/multiplayer/ClientPacketListener;send(Lnet/minecraft/network/protocol/Packet;)V"))
    private void myth$positionItem(ClientPacketListener connection, Packet<?> packet, Operation<Void> original) {
        // 26.2 carries use-item rotation in this packet instead of a preceding movement packet.
        if (packet instanceof ServerboundUseItemPacket) {
            PositionItemEvent event = Myth.EVENT_MANAGER.call(new PositionItemEvent(packet));
            if (!event.isCancelled()) {
                original.call(connection, event.getPacket());
            }
        } else {
            original.call(connection, packet);
        }
    }
}
