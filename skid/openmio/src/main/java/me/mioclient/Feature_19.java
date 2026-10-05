package me.mioclient;

import com.mojang.brigadier.arguments.StringArgumentType;
import net.minecraft.command.CommandSource;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_19.class */
public class Feature_19 extends Feature {
    public Feature_19() {
        super("glint");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.literal("clear").executes(commandContext -> {
            BaritoneHelper_3.items.getEntityListObjectSetting1123().getValue().clear();
            return 1;
        })).then(Feature.argument("item", StringArgumentType.string()).suggests((commandContext2, suggestionsBuilder) -> {
            return CommandSource.suggestMatching(BaritoneHelper_3.items.getEntityListObjectSetting1123().getCollection3127(), suggestionsBuilder);
        }).executes(commandContext3 -> {
            try {
                BaritoneHelper_3.items.getEntityListObjectSetting1123().do134((String) commandContext3.getArgument("item", String.class));
                return 1;
            } catch (Exception e) {
                return 1;
            }
        }));
    }
}
