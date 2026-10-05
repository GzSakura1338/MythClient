package me.mioclient;

import com.mojang.brigadier.arguments.BoolArgumentType;
import me.mioclient.module.Module;
import net.minecraft.command.CommandSource;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_3.class */
public final class Feature_3 extends Feature {
    public Feature_3() {
        super("toggle");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.argument("module", new ArgumentType_8()).then(Feature.argument("state", BoolArgumentType.bool()).executes(commandContext -> {
            ((Module) commandContext.getArgument("module", Module.class)).do495(((Boolean) commandContext.getArgument("state", Boolean.class)).booleanValue());
            return 1;
        })).executes(commandContext2 -> {
            ((Module) commandContext2.getArgument("module", Module.class)).do496();
            return 1;
        }));
    }
}
