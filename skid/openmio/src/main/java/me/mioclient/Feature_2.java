package me.mioclient;

import com.mojang.brigadier.arguments.FloatArgumentType;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_2.class */
public final class Feature_2 extends Feature {
    public Feature_2() {
        super("pitch");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.argument("pitch", FloatArgumentType.floatArg(-FreecamHelper.num2, FreecamHelper.num2)).executes(commandContext -> {
            minecraftClient.player.setPitch(((Float) commandContext.getArgument("pitch", Float.class)).floatValue());
            MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(String.valueOf(commandContext.getArgument("pitch", Float.class))).getString2921("Player's pitch has been set to \u0001.")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        }));
    }
}
