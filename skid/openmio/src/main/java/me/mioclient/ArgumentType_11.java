package me.mioclient;

import com.mojang.brigadier.ImmutableStringReader;
import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.exceptions.CommandSyntaxException;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.util.Arrays;
import java.util.concurrent.CompletableFuture;
import net.minecraft.command.CommandSource;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ArgumentType_11.class */
public class ArgumentType_11 implements com.mojang.brigadier.arguments.ArgumentType<String> {
    public final String[] stringArr;

    public ArgumentType_11(String... strArr) {
        this.stringArr = strArr;
    }

    /* renamed from: parse, reason: merged with bridge method [inline-methods] */
    public String parse(StringReader stringReader) throws CommandSyntaxException {
        String readString = stringReader.readString();
        for (String str : this.stringArr) {
            if (str.equals(readString)) {
                return str;
            }
        }
        throw CommandSyntaxException.BUILT_IN_EXCEPTIONS.literalIncorrect().createWithContext((ImmutableStringReader) stringReader, Arrays.toString(this.stringArr));
    }

    public <S> CompletableFuture<Suggestions> listSuggestions(CommandContext<S> commandContext, SuggestionsBuilder suggestionsBuilder) {
        String remaining = suggestionsBuilder.getRemaining();
        for (String str : this.stringArr) {
            if (str.equals(remaining)) {
                suggestionsBuilder.suggest(str);
                return suggestionsBuilder.buildFuture();
            }
        }
        if (CommandSource.shouldSuggest(suggestionsBuilder.getRemaining(), this.stringArr[0])) {
            suggestionsBuilder.suggest(this.stringArr[0]);
        }
        return suggestionsBuilder.buildFuture();
    }
}
