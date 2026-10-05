package me.mioclient;

import com.mojang.brigadier.arguments.StringArgumentType;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_26.class */
public final class Feature_26 extends Feature {
    public Feature_26() {
        super("ignore");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.literal("add").then(Feature.argument("name", StringArgumentType.word()).executes(commandContext -> {
            String lowerCase = ((String) commandContext.getArgument("name", String.class)).toLowerCase();
            if (BaritoneHelper_3.nameTagsSearchHelper4.getSet2312().contains(lowerCase)) {
                MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(lowerCase).getString2921("You are already ignoring \u0001")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
                return 1;
            }
            BaritoneHelper_3.nameTagsSearchHelper4.getSet2312().add(lowerCase);
            MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(lowerCase).getString2921("Player \u0001 has been ignored")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            return 1;
        }))).then(Feature.literal("remove").then(Feature.argument("name", StringArgumentType.word()).suggests((commandContext2, suggestionsBuilder) -> {
            return CommandSource.suggestMatching(BaritoneHelper_3.nameTagsSearchHelper4.getSet2312(), suggestionsBuilder);
        }).executes(commandContext3 -> {
            String lowerCase = ((String) commandContext3.getArgument("name", String.class)).toLowerCase();
            if (!BaritoneHelper_3.nameTagsSearchHelper4.getSet2312().contains(lowerCase)) {
                MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(lowerCase).getString2921("You are not ignoring \u0001")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
                return 1;
            }
            MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(lowerCase).getString2921("\u0001 is no longer ignored")), MixinMessageIndicatorHelper.getMessageSignatureData337(-1));
            BaritoneHelper_3.nameTagsSearchHelper4.getSet2312().remove(lowerCase);
            return 1;
        })));
    }
}
