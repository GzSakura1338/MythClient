package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.player.NameProtect;
import net.minecraft.client.MinecraftClient;
import net.minecraft.text.TextVisitFactory;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.ModifyArg;

/* compiled from: 0.java */
@Mixin({TextVisitFactory.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinTextVisitFactory.class */
public class MixinTextVisitFactory {
    private static NameProtect nameprotect = (NameProtect) BaritoneHelper_3.baritoneHelper_4.getModule117(NameProtect.class);

    @ModifyArg(method = {"visitFormatted(Ljava/lang/String;ILnet/minecraft/text/Style;Lnet/minecraft/text/CharacterVisitor;)Z"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/text/TextVisitFactory;visitFormatted(Ljava/lang/String;ILnet/minecraft/text/Style;Lnet/minecraft/text/Style;Lnet/minecraft/text/CharacterVisitor;)Z", ordinal = 0), index = 0)
    private static String adjustText(String str) {
        if (nameprotect.isToggled() && MinecraftClient.getInstance().world != null) {
            str = str.replace(MinecraftClient.getInstance().getSession().getUsername(), nameprotect.name.getValue());
        }
        return str;
    }
}
