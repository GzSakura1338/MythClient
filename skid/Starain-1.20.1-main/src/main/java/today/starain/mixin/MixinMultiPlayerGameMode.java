package today.starain.mixin;

import net.minecraft.client.multiplayer.ClientPacketListener;
import net.minecraft.client.multiplayer.MultiPlayerGameMode;
import net.minecraft.network.protocol.Packet;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Redirect;
import today.starain.Starain;
import today.starain.event.impl.PositionItemEvent;

@Mixin({MultiPlayerGameMode.class})
public class MixinMultiPlayerGameMode {
    @Redirect(
            method = {"useItem"},
            at = @At(
                    value = "INVOKE",
                    target = "Lnet/minecraft/client/multiplayer/ClientPacketListener;send(Lnet/minecraft/network/protocol/Packet;)V",
                    ordinal = 0
            )
    )
    public void onSendPacket(ClientPacketListener instance, Packet<?> pPacket) {
        PositionItemEvent event = new PositionItemEvent(pPacket);
        Starain.getInstance().getEventManager().call(event);
        if (!event.isCancelled()) {
            instance.send(event.getPacket());
        }
    }

//    @Inject(
//            method = {"startDestroyBlock"},
//            at = {@At("HEAD")}
//    )
//    public void onStartDestroyBlock(BlockPos pLoc, Direction pFace, CallbackInfoReturnable<Boolean> cir) {
//        Naven.getInstance().getEventManager().call(new EventDestroyBlock(pLoc, pFace));
//    }
}
