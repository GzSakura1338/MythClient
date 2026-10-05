package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.misc.CustomDeathText;
import net.minecraft.client.gui.screen.DeathScreen;
import net.minecraft.text.Text;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Mutable;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({DeathScreen.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinDeathScreen.class */
public class MixinDeathScreen {
    private static CustomDeathText customdeath = (CustomDeathText) BaritoneHelper_3.baritoneHelper_4.getModule117(CustomDeathText.class);

    @Mutable
    @Shadow
    @Final
    private Text field_2450;

    @Inject(method = {"<init>"}, at = {@At("RETURN")})
    public void mio$init(Text text, boolean z, CallbackInfo callbackInfo) {
        if (customdeath == null || !customdeath.isToggled()) {
            return;
        }
        this.field_2450 = customdeath.getText327();
    }
}
