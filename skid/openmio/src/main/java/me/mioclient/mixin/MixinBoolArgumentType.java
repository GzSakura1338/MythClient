package me.mioclient.mixin;

import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.arguments.BoolArgumentType;
import com.mojang.brigadier.exceptions.CommandSyntaxException;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({BoolArgumentType.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinBoolArgumentType.class */
public class MixinBoolArgumentType {
    @Inject(method = {"parse(Lcom/mojang/brigadier/StringReader;)Ljava/lang/Boolean;"}, at = {@At("HEAD")}, remap = false, cancellable = true)
    private void parseHook(StringReader stringReader, CallbackInfoReturnable<Boolean> callbackInfoReturnable) throws com.mojang.brigadier.exceptions.CommandSyntaxException {
        String readString = stringReader.readString();
        if (readString.equals("0") || readString.equalsIgnoreCase("false")) {
            callbackInfoReturnable.setReturnValue(false);
        } else {
            if (!readString.equals("1") && !readString.equalsIgnoreCase("true")) {
                throw CommandSyntaxException.BUILT_IN_EXCEPTIONS.readerInvalidBool().createWithContext(stringReader, readString);
            }
            callbackInfoReturnable.setReturnValue(true);
        }
    }
}
