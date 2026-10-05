package today.starain.mixin;

import net.minecraft.client.multiplayer.ClientPacketListener;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import today.starain.Starain;
import today.starain.event.impl.ChatEvent;

import java.util.Objects;

@Mixin(ClientPacketListener.class)
public abstract class MixinClientPacketListener {
    @Unique
    private boolean starain$skipChatEvent;

    @Shadow
    public abstract void sendChat(String msg);

    @Inject(method = "sendChat(Ljava/lang/String;)V", at = @At("HEAD"), cancellable = true)
    private void starain$sendChatMixin(String msg, CallbackInfo ci) {
        if (starain$skipChatEvent
                || Starain.getInstance() == null
                || Starain.getInstance().getEventManager() == null) {
            return;
        }

        ChatEvent event = new ChatEvent(msg);
        Starain.getInstance().getEventManager().call(event);

        if (event.isCancelled()) {
            ci.cancel();
            return;
        }

        if (!Objects.equals(event.getMessage(), msg)) {
            starain$skipChatEvent = true;
            try {
                sendChat(event.getMessage());
            } finally {
                starain$skipChatEvent = false;
            }
            ci.cancel();
        }
    }
}
