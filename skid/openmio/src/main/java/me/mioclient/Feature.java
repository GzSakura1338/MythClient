package me.mioclient;

import com.mojang.brigadier.builder.ArgumentBuilder;
import com.mojang.brigadier.builder.RequiredArgumentBuilder;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.util.concurrent.CompletableFuture;
import net.minecraft.command.CommandSource;
import net.minecraft.text.MutableText;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Feature.class */
public abstract class Feature extends me.mioclient.module.Feature {
    public String[] aliases;

    public Feature(String str) {
        super(str);
        this.aliases = new String[0];
    }

    public static <T> RequiredArgumentBuilder<CommandSource, T> argument(String str, com.mojang.brigadier.arguments.ArgumentType<T> argumentType) {
        return RequiredArgumentBuilder.argument(str, argumentType);
    }

    public static com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literal(String str) {
        return com.mojang.brigadier.builder.LiteralArgumentBuilder.literal(str);
    }

    public static RequiredArgumentBuilder<CommandSource, String> getRequiredArgumentBuilder411(String... strArr) {
        return argument(strArr[0], new ArgumentType_11(strArr));
    }

    public static CompletableFuture<Suggestions> getCompletableFuture412(SuggestionsBuilder suggestionsBuilder, String... strArr) {
        for (String str : strArr) {
            suggestionsBuilder.suggest(str);
        }
        return suggestionsBuilder.buildFuture();
    }

    public void do413(java.lang.Runnable runnable) {
        BaritoneHelper_3.tooltipsSearchHelper4_2.do164(runnable, 0);
    }

    public MutableText getMutableText341(String str) {
        return Text.empty().append(MixinMessageIndicatorHelper.getMutableText341(str));
    }

    public void do414(String... strArr) {
        this.aliases = strArr;
    }

    public String[] getAliases() {
        return this.aliases;
    }

    public abstract void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder);

    public void do415(Feature feature, com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
        ArgumentBuilder literal = literal(feature.getName());
        feature.exec((com.mojang.brigadier.builder.LiteralArgumentBuilder) literal);
        literalArgumentBuilder.then(literal);
    }
}
