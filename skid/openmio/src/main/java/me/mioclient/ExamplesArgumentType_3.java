package me.mioclient;

import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.exceptions.CommandSyntaxException;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.util.Collection;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import net.minecraft.command.CommandSource;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ExamplesArgumentType_3.class */
public class ExamplesArgumentType_3 implements com.mojang.brigadier.arguments.ArgumentType<String>, SearchHelper_4 {
    public static final List<String> list = List.of("cat", "fit", "asphyxia1337");

    /* renamed from: parse, reason: merged with bridge method [inline-methods] */
    public String parse(StringReader stringReader) throws CommandSyntaxException {
        return stringReader.readString();
    }

    public <S> CompletableFuture<Suggestions> listSuggestions(CommandContext<S> commandContext, SuggestionsBuilder suggestionsBuilder) {
        return CommandSource.suggestMatching(minecraftClient.player.networkHandler.getPlayerList().stream().map(playerListEntry -> {
            return playerListEntry.getProfile().getName();
        }), suggestionsBuilder);
    }

    public Collection<String> getExamples() {
        return list;
    }
}
