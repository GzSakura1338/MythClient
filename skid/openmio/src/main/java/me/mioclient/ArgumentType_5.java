package me.mioclient;

import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.exceptions.CommandSyntaxException;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.util.ArrayList;
import java.util.Collection;
import java.util.concurrent.CompletableFuture;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ArgumentType_5.class */
public class ArgumentType_5<T> implements com.mojang.brigadier.arguments.ArgumentType<Collection<T>> {
    public final com.mojang.brigadier.arguments.ArgumentType<T> argumentType;

    public ArgumentType_5(com.mojang.brigadier.arguments.ArgumentType<T> argumentType) {
        this.argumentType = argumentType;
    }

    /* renamed from: parse, reason: merged with bridge method [inline-methods] */
    public Collection<T> parse(StringReader stringReader) throws CommandSyntaxException {
        String remaining = stringReader.getRemaining();
        stringReader.setCursor(stringReader.getTotalLength());
        String[] split = remaining.split(" ");
        ArrayList arrayList = new ArrayList();
        for (String str : split) {
            arrayList.add(this.argumentType.parse(new StringReader(str)));
        }
        return arrayList;
    }

    public <S> CompletableFuture<Suggestions> listSuggestions(CommandContext<S> commandContext, SuggestionsBuilder suggestionsBuilder) {
        String[] split = suggestionsBuilder.getRemaining().split(" ");
        int start = (suggestionsBuilder.getStart() + suggestionsBuilder.getRemaining().length()) - split[split.length - 1].length();
        return this.argumentType.listSuggestions(commandContext, new SuggestionsBuilder(suggestionsBuilder.getInput(), start));
    }
}
