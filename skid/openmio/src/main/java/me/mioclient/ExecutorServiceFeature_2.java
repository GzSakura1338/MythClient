package me.mioclient;

import com.mojang.brigadier.arguments.StringArgumentType;
import java.time.Duration;
import java.time.OffsetDateTime;
import java.time.ZoneOffset;
import java.time.format.DateTimeFormatter;
import java.util.Locale;
import me.mioclient.MixinMessageIndicatorHelper;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;
import net.minecraft.util.Formatting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ExecutorServiceFeature_2.class */
public class ExecutorServiceFeature_2 extends ExecutorServiceFeature {
    public final DateTimeFormatter dateTimeFormatter;

    public ExecutorServiceFeature_2() {
        super("seen");
        this.dateTimeFormatter = DateTimeFormatter.ofPattern("dd MMM yyyy HH:mm:ss", Locale.ENGLISH);
        do414("lastseen");
    }

    @Override // me.mioclient.ExecutorServiceFeature, me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        literalArgumentBuilder.then(Feature.argument("name", StringArgumentType.greedyString()).executes(commandContext -> {
            String str = (String) commandContext.getArgument("name", String.class);
            MixinMessageIndicatorHelper.do344(Text.of("Fetching data..."), MixinMessageIndicatorHelper.getMessageSignatureData338(this));
            getCompletableFuture1449("seen", str).whenComplete((jsonObject, th) -> {
                String message;
                if (th != null) {
                    message = th.getMessage();
                } else {
                    try {
                        if (jsonObject.has("lastSeen")) {
                            if (jsonObject.get("lastSeen").isJsonNull()) {
                                throw new java.lang.RuntimeException("No data for player.");
                            }
                            OffsetDateTime parse = OffsetDateTime.parse(jsonObject.get("lastSeen").getAsString());
                            Duration between = Duration.between(parse, OffsetDateTime.now(ZoneOffset.UTC));
                            StringBuilder sb = new StringBuilder(str);
                            if (((int) between.toSeconds()) == 0) {
                                sb.append(" is online right now.");
                            } else {
                                sb.append(" was seen ");
                                sb.append(ExecutorServiceFeature.getString1450((int) between.toSeconds()));
                                sb.append(" ago (");
                                sb.append(this.dateTimeFormatter.format(parse));
                                sb.append(").");
                            }
                            MixinMessageIndicatorHelper.do344(Text.of(sb.toString()), MixinMessageIndicatorHelper.getMessageSignatureData338(this));
                            return;
                        }
                        message = "Response data doesn't have a required field: lastSeen.";
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
