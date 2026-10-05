package me.mioclient;

import com.mojang.brigadier.arguments.StringArgumentType;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;
import net.minecraft.util.Formatting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_16.class */
public final class Feature_16 extends Feature {
    public Feature_16() {
        super("prefix");
        baritoneHelper.do1796(this);
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.argument("prefix", StringArgumentType.greedyString()).executes(commandContext -> {
            ChatFilterSearchHelper4_2.do2983((String) commandContext.getArgument("prefix", String.class));
            MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(ChatFilterSearchHelper4_2.getString2982()).getArgumentTypeHelper2919(String.valueOf(Formatting.GRAY)).getString2921("\u0001Prefix set to \u0001")), MixinMessageIndicatorHelper.getMessageSignatureData337(1));
            return 1;
        })).executes(commandContext2 -> {
            MixinMessageIndicatorHelper.do344(Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(ChatFilterSearchHelper4_2.getString2982()).getArgumentTypeHelper2919(String.valueOf(Formatting.GRAY)).getString2921("\u0001The current prefix is \u0001")), MixinMessageIndicatorHelper.getMessageSignatureData337(1));
            return 1;
        });
    }
}
