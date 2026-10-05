package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.WrapWithCondition;
import me.mioclient.SearchHelper_4;
import me.mioclient.event.InsertItemEvent;
import net.minecraft.screen.ScreenHandler;
import net.minecraft.screen.slot.Slot;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;

/* compiled from: 0.java */
@Mixin({ScreenHandler.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinScreenHandler.class */
public class MixinScreenHandler {
    @WrapWithCondition(method = {"insertItem"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/screen/slot/Slot;markDirty()V")})
    private boolean insertItemHook(Slot slot) {
        SearchHelper_4.baritoneHelper.getObject1794(new InsertItemEvent(slot));
        return true;
    }
}
