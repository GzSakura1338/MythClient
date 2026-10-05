package me.mioclient;

import com.mojang.brigadier.arguments.StringArgumentType;
import net.minecraft.command.CommandSource;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_37.class */
public final class Feature_37 extends Feature {
    public Feature_37() {
        super("say");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.argument("message", StringArgumentType.greedyString()).executes(commandContext -> {
            MixinMessageIndicatorHelper.do347((String) commandContext.getArgument("message", String.class));
            return 1;
        }));
    }
}
