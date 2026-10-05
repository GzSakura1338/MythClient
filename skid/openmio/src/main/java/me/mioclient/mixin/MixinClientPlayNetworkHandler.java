package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.SearchHelper_4;
import me.mioclient.event.ChunkDeltaUpdateEvent;
import me.mioclient.event.ExplosionVelocityEvent;
import me.mioclient.event.UpdateSelectedSlotEvent;
import me.mioclient.mixin.ducks.DuckExplosionS2CPacket;
import me.mioclient.module.movement.Velocity;
import me.mioclient.module.render.NoRender;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.network.ClientPlayNetworkHandler;
import net.minecraft.network.packet.s2c.play.BlockUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.ChunkDeltaUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.EntitySpawnS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityVelocityUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.ExplosionS2CPacket;
import net.minecraft.network.packet.s2c.play.UpdateSelectedSlotS2CPacket;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({ClientPlayNetworkHandler.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinClientPlayNetworkHandler.class */
public class MixinClientPlayNetworkHandler {
    private static NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);
    private static Velocity velocity = (Velocity) BaritoneHelper_3.baritoneHelper_4.getModule117(Velocity.class);

    @Inject(method = {"onExplosion"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/network/NetworkThreadUtils;forceMainThread(Lnet/minecraft/network/packet/Packet;Lnet/minecraft/network/listener/PacketListener;Lnet/minecraft/util/thread/ThreadExecutor;)V", shift = At.Shift.AFTER)}, cancellable = true)
    private void onExplosionVelocity(ExplosionS2CPacket explosionS2CPacket, CallbackInfo callbackInfo) {
        ExplosionVelocityEvent explosionVelocityEvent = new ExplosionVelocityEvent(explosionS2CPacket);
        SearchHelper_4.baritoneHelper.getObject1794(explosionVelocityEvent);
        if (explosionVelocityEvent.is2403()) {
            callbackInfo.cancel();
            return;
        }
        DuckExplosionS2CPacket duckExplosionS2CPacket = (DuckExplosionS2CPacket) explosionS2CPacket;
        duckExplosionS2CPacket.setX(explosionVelocityEvent.get767());
        duckExplosionS2CPacket.setY(explosionVelocityEvent.get769());
        duckExplosionS2CPacket.setZ(explosionVelocityEvent.get771());
    }

    @Inject(method = {"onEntityVelocityUpdate"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/network/NetworkThreadUtils;forceMainThread(Lnet/minecraft/network/packet/Packet;Lnet/minecraft/network/listener/PacketListener;Lnet/minecraft/util/thread/ThreadExecutor;)V", shift = At.Shift.AFTER)}, cancellable = true)
    private void onEntityVelocityUpdate(EntityVelocityUpdateS2CPacket entityVelocityUpdateS2CPacket, CallbackInfo callbackInfo) {
        if (velocity.is1893() && entityVelocityUpdateS2CPacket.getEntityId() == MinecraftClient.getInstance().player.getId()) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"onEntitySpawn"}, at = {@At("HEAD")}, cancellable = true)
    private void onEntitySpawn(EntitySpawnS2CPacket entitySpawnS2CPacket, CallbackInfo callbackInfo) {
        if (norender.isToggled() && entitySpawnS2CPacket != null && entitySpawnS2CPacket.getEntityType() != null && norender.is1993(entitySpawnS2CPacket.getEntityType()) && norender.removal.getValue() == NoRender.NoRenderMode.FULL) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"onUpdateSelectedSlot"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/network/NetworkThreadUtils;forceMainThread(Lnet/minecraft/network/packet/Packet;Lnet/minecraft/network/listener/PacketListener;Lnet/minecraft/util/thread/ThreadExecutor;)V", shift = At.Shift.AFTER)}, cancellable = true)
    private void onUpdateSelectedSlot(UpdateSelectedSlotS2CPacket updateSelectedSlotS2CPacket, CallbackInfo callbackInfo) {
        UpdateSelectedSlotEvent updateSelectedSlotEvent = new UpdateSelectedSlotEvent(updateSelectedSlotS2CPacket.getSlot());
        SearchHelper_4.baritoneHelper.getObject1794(updateSelectedSlotEvent);
        if (updateSelectedSlotEvent.is2403()) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"onBlockUpdate"}, at = {@At("TAIL")})
    private void onBlockUpdateHook(BlockUpdateS2CPacket blockUpdateS2CPacket, CallbackInfo callbackInfo) {
        SearchHelper_4.baritoneHelper.getObject1794(new ChunkDeltaUpdateEvent());
    }

    @Inject(method = {"onChunkDeltaUpdate"}, at = {@At("TAIL")})
    private void onChunkDeltaUpdateHook(ChunkDeltaUpdateS2CPacket chunkDeltaUpdateS2CPacket, CallbackInfo callbackInfo) {
        SearchHelper_4.baritoneHelper.getObject1794(new ChunkDeltaUpdateEvent());
    }
}
