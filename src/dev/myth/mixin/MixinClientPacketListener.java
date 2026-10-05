package dev.myth.mixin;

import com.llamalad7.mixinextras.injector.wrapmethod.WrapMethod;
import com.llamalad7.mixinextras.injector.wrapoperation.Operation;
import dev.myth.Myth;
import dev.myth.event.impl.ChatEvent;
import net.minecraft.client.multiplayer.ClientPacketListener;
import org.spongepowered.asm.mixin.Mixin;

@Mixin(ClientPacketListener.class)
public abstract class MixinClientPacketListener {
    @WrapMethod(method = "sendChat")
    private void myth$sendChat(String message, Operation<Void> original) {
        ChatEvent event = Myth.EVENT_MANAGER.call(new ChatEvent(message));
        if (!event.isCancelled()) {
            original.call(event.getMessage());
        }
    }
}
