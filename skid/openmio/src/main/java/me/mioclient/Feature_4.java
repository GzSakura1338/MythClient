package me.mioclient;

import com.mojang.brigadier.arguments.StringArgumentType;
import java.net.URL;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature_4.class */
public class Feature_4 extends Feature {
    public Feature_4() {
        super("webhook");
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.literal("get").executes(commandContext -> {
            String string1200 = BaritoneHelper_3.discordNotifsHelper.getString1200();
            if (string1200 == null || string1200.isEmpty()) {
                MixinMessageIndicatorHelper.do344(Text.literal("You don't have a webhook URL set"), MixinMessageIndicatorHelper.getMessageSignatureData337(-1035361));
                return 1;
            }
            MixinMessageIndicatorHelper.do344(Text.literal("Your webhook URL is ").append(getMutableText341(string1200)), MixinMessageIndicatorHelper.getMessageSignatureData337(-1035361));
            return 1;
        })).then(Feature.literal("set").then(Feature.argument("url", StringArgumentType.greedyString()).executes(commandContext2 -> {
            String str = (String) commandContext2.getArgument("url", String.class);
            if (!str.startsWith("http://") && !str.startsWith("https://")) {
                str = new ArgumentTypeHelper().getArgumentTypeHelper2919(str).getString2921("https://\u0001");
            }
            try {
                new URL(str);
                BaritoneHelper_3.discordNotifsHelper.do1199(str);
                MixinMessageIndicatorHelper.do344(Text.literal("Your webhook URL has been updated to ").append(getMutableText341(str)), MixinMessageIndicatorHelper.getMessageSignatureData337(-1035362));
                return 1;
            } catch (Exception e) {
                MixinMessageIndicatorHelper.do344(Text.literal("The provided webhook URL is invalid"), MixinMessageIndicatorHelper.getMessageSignatureData337(-1035362));
                return 1;
            }
        }))).then(Feature.literal("clear").executes(commandContext3 -> {
            BaritoneHelper_3.discordNotifsHelper.do1199("");
            MixinMessageIndicatorHelper.do344(Text.literal("Your webhook URL has been cleared out"), MixinMessageIndicatorHelper.getMessageSignatureData337(-1035363));
            return 1;
        }));
    }
}
