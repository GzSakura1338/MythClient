package me.mioclient.mixin;

import com.llamalad7.mixinextras.sugar.Local;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.ChatFilterSearchHelper4_2;
import me.mioclient.SearchHelper_4;
import me.mioclient.Vec3dEvent;
import me.mioclient.feature.Event;
import me.mioclient.module.render.Ambience;
import net.minecraft.client.network.ClientPlayNetworkHandler;
import net.minecraft.client.world.ClientWorld;
import net.minecraft.entity.Entity;
import net.minecraft.network.packet.s2c.play.VehicleMoveS2CPacket;
import net.minecraft.network.packet.s2c.play.WorldTimeUpdateS2CPacket;
import net.minecraft.util.math.Vec3d;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.ModifyVariable;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({ClientPlayNetworkHandler.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinClientNetworkHandler.class */
public class MixinClientNetworkHandler {
    private static final Ambience ambience = (Ambience) BaritoneHelper_3.baritoneHelper_4.getModule117(Ambience.class);

    @Shadow
    private ClientWorld field_3699;

    @Unique
    private Event event;

    @Inject(method = {"sendChatMessage"}, at = {@At("HEAD")}, cancellable = true)
    private void sendChatMessageHook(String str, CallbackInfo callbackInfo) {
        if (str.startsWith(ChatFilterSearchHelper4_2.getString2982())) {
            ChatFilterSearchHelper4_2.do2060(str.substring(ChatFilterSearchHelper4_2.getString2982().length()));
            callbackInfo.cancel();
        }
    }

    @ModifyVariable(method = {"sendChatMessage"}, at = @At(value = "INVOKE", target = "Ljava/time/Instant;now()Ljava/time/Instant;", shift = At.Shift.BEFORE), argsOnly = true)
    private String dabigbulletz(String str) {
        this.event = new Event(str);
        SearchHelper_4.baritoneHelper.getObject1794(this.event);
        return this.event.getString2649();
    }

    @Inject(method = {"sendChatMessage"}, at = {@At(value = "INVOKE", target = "Ljava/time/Instant;now()Ljava/time/Instant;", shift = At.Shift.AFTER)}, cancellable = true)
    private void dabigbulletz(String str, CallbackInfo callbackInfo) {
        if (this.event == null || !this.event.is2403()) {
            return;
        }
        callbackInfo.cancel();
    }

    @Inject(method = {"onWorldTimeUpdate"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/network/NetworkThreadUtils;forceMainThread(Lnet/minecraft/network/packet/Packet;Lnet/minecraft/network/listener/PacketListener;Lnet/minecraft/util/thread/ThreadExecutor;)V", shift = At.Shift.AFTER)}, cancellable = true)
    private void onWorldTimeUpdateHook(WorldTimeUpdateS2CPacket worldTimeUpdateS2CPacket, CallbackInfo callbackInfo) {
        if (ambience.isToggled() && ambience.worldTime.getValue().booleanValue()) {
            this.field_3699.setTime(ambience.get2923());
            this.field_3699.setTimeOfDay(ambience.get2923());
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"onVehicleMove"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/entity/Entity;updatePositionAndAngles(DDDFF)V")}, cancellable = true)
    private void onVehicleMove(VehicleMoveS2CPacket vehicleMoveS2CPacket, CallbackInfo callbackInfo, @Local Entity entity) {
        Vec3dEvent vec3dEvent = new Vec3dEvent(new Vec3d(vehicleMoveS2CPacket.getX(), vehicleMoveS2CPacket.getY(), vehicleMoveS2CPacket.getZ()));
        SearchHelper_4.baritoneHelper.getObject1794(vec3dEvent);
        if (vec3dEvent.is2403()) {
            callbackInfo.cancel();
        }
    }
}
