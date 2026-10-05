package me.mioclient;

import com.mojang.brigadier.ImmutableStringReader;
import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.exceptions.CommandSyntaxException;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.util.Arrays;
import java.util.Collection;
import java.util.concurrent.CompletableFuture;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ExamplesArgumentType_2.class */
public class ExamplesArgumentType_2 implements com.mojang.brigadier.arguments.ArgumentType<Boolean> {
    public static final Collection<String> collection = Arrays.asList("true", "false", "toggle");
    public final BooleanSetting booleanSetting;

    public ExamplesArgumentType_2(BooleanSetting booleanSetting) {
        this.booleanSetting = booleanSetting;
    }

    public static ExamplesArgumentType_2 bool(BooleanSetting booleanSetting) {
        return new ExamplesArgumentType_2(booleanSetting);
    }

    public static boolean getBool(CommandContext<?> commandContext, String str) {
        return ((Boolean) commandContext.getArgument(str, Boolean.class)).booleanValue();
    }

    /* renamed from: parse, reason: merged with bridge method [inline-methods] */
    public Boolean parse(StringReader stringReader) throws CommandSyntaxException {
        String readString = stringReader.readString();
        if (readString.equals("0") || readString.equalsIgnoreCase("false")) {
            return false;
        }
        if (readString.equals("1") || readString.equalsIgnoreCase("true")) {
            return true;
        }
        if (readString.equalsIgnoreCase("toggle")) {
            return Boolean.valueOf(!this.booleanSetting.getValue().booleanValue());
        }
        throw CommandSyntaxException.BUILT_IN_EXCEPTIONS.readerInvalidBool().createWithContext((ImmutableStringReader) stringReader, readString);
    }

    public <S> CompletableFuture<Suggestions> listSuggestions(CommandContext<S> commandContext, SuggestionsBuilder suggestionsBuilder) {
        if ("true".startsWith(suggestionsBuilder.getRemainingLowerCase())) {
            suggestionsBuilder.suggest("true");
        }
        if ("false".startsWith(suggestionsBuilder.getRemainingLowerCase())) {
            suggestionsBuilder.suggest("false");
        }
        if ("toggle".startsWith(suggestionsBuilder.getRemainingLowerCase())) {
            suggestionsBuilder.suggest("toggle");
        }
        return suggestionsBuilder.buildFuture();
    }

    public Collection<String> getExamples() {
        return collection;
    }
}
