package me.mioclient;

import com.mojang.brigadier.arguments.StringArgumentType;
import me.mioclient.MixinMessageIndicatorHelper;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;
import net.minecraft.util.Formatting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ExecutorServiceFeature_3.class */
public class ExecutorServiceFeature_3 extends ExecutorServiceFeature {
    public ExecutorServiceFeature_3() {
        super("playtime");
        do414("pt");
    }

    @Override // me.mioclient.ExecutorServiceFeature, me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.argument("name", StringArgumentType.greedyString()).executes(commandContext -> {
            String str = (String) commandContext.getArgument("name", String.class);
            MixinMessageIndicatorHelper.do344(Text.of("Fetching data..."), MixinMessageIndicatorHelper.getMessageSignatureData338(this));
            getCompletableFuture1449("playtime", str).whenComplete((jsonObject, th) -> {
                String message;
                if (th != null) {
                    message = th.getMessage();
                } else {
                    try {
                        if (jsonObject.has("playtimeSeconds")) {
                            if (jsonObject.get("playtimeSeconds").isJsonNull()) {
                                throw new java.lang.RuntimeException("No data for player.");
                            }
                            MixinMessageIndicatorHelper.do344(Text.of(str + " has played for " + ExecutorServiceFeature.getString1450(jsonObject.get("playtimeSeconds").getAsInt()) + "."), MixinMessageIndicatorHelper.getMessageSignatureData338(this));
                            return;
                        }
                        message = "Response data doesn't have a required field: playtimeSeconds.";
                    } catch (Exception e) {
                        message = e.getMessage();
                    }
                }
                if (message != null) {
                    MixinMessageIndicatorHelper.do345(Text.of(new ArgumentTypeHelper().getArgumentTypeHelper2919(message).getArgumentTypeHelper2919(String.valueOf(Formatting.RED)).getString2921("\u0001\u0001")), MixinMessageIndicatorHelper.getMessageSignatureData338(this), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode2);
                }
            });
            return 1;
        }));
    }
}
