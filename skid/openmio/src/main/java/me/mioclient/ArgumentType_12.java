package me.mioclient;

import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.exceptions.CommandSyntaxException;
import com.mojang.brigadier.exceptions.DynamicCommandExceptionType;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ArgumentType_12.class */
public class ArgumentType_12 implements com.mojang.brigadier.arguments.ArgumentType<Data> {
    /* renamed from: parse, reason: merged with bridge method [inline-methods] */
    public Data parse(StringReader stringReader) throws CommandSyntaxException {
        String readString = stringReader.readString();
        Optional<Data> optional2679 = BaritoneHelper_3.chatFilterSearchHelper4.getOptional2679(readString);
        if (optional2679.isEmpty()) {
            throw new DynamicCommandExceptionType(obj -> {
                return Text.of(new ArgumentTypeHelper().getArgumentTypeHelper2919(String.valueOf(obj)).getString2921("Filter not found \u0001"));
            }).create(readString);
        }
        return optional2679.get();
    }

    public <S> CompletableFuture<Suggestions> listSuggestions(CommandContext<S> commandContext, SuggestionsBuilder suggestionsBuilder) {
        return CommandSource.suggestMatching(BaritoneHelper_3.chatFilterSearchHelper4.getStream2678(), suggestionsBuilder);
    }
}
