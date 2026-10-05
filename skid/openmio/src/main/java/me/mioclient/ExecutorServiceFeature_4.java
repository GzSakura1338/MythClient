package me.mioclient;

import me.mioclient.MixinMessageIndicatorHelper;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;
import net.minecraft.util.Formatting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ExecutorServiceFeature_4.class */
public class ExecutorServiceFeature_4 extends ExecutorServiceFeature {
    public ExecutorServiceFeature_4() {
        super("queue");
    }

    @Override // me.mioclient.ExecutorServiceFeature, me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.executes(commandContext -> {
            MixinMessageIndicatorHelper.do344(Text.of("Fetching data..."), MixinMessageIndicatorHelper.getMessageSignatureData338(this));
            getCompletableFuture1449("queue", (String) null).whenComplete((jsonObject, th) -> {
                String message;
                if (th != null) {
                    message = th.getMessage();
                } else {
                    try {
                        int i = -1337;
                        int i2 = -1337;
                        if (jsonObject.has("prio")) {
                            i = jsonObject.get("prio").getAsInt();
                        }
                        if (jsonObject.has("regular")) {
                            i2 = jsonObject.get("regular").getAsInt();
                        }
                        StringBuilder sb = new StringBuilder();
                        if (i2 != -1337) {
                            sb.append("%d in normal queue".formatted(Integer.valueOf(i2)));
                        }
                        if (i2 != -1337 && i != -1337) {
                            sb.append(", ");
                        }
                        if (i != -1337) {
                            sb.append("%d in priority queue".formatted(Integer.valueOf(i)));
                        }
                        MixinMessageIndicatorHelper.do344(Text.of(sb.toString()), MixinMessageIndicatorHelper.getMessageSignatureData338(this));
                        return;
                    } catch (Exception e) {
                        message = e.getMessage();
                    }
                }
                if (message != null) {
                    MixinMessageIndicatorHelper.do345(Text.of(new ArgumentTypeHelper().getArgumentTypeHelper2919(message).getArgumentTypeHelper2919(String.valueOf(Formatting.RED)).getString2921("\u0001\u0001")), MixinMessageIndicatorHelper.getMessageSignatureData338(this), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode2);
                }
            });
            return 1;
        });
    }
}
