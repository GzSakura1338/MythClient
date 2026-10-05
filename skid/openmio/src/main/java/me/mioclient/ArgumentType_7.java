package me.mioclient;

import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.exceptions.CommandSyntaxException;
import com.mojang.brigadier.exceptions.DynamicCommandExceptionType;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.util.concurrent.CompletableFuture;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ArgumentType_7.class */
public class ArgumentType_7 implements com.mojang.brigadier.arguments.ArgumentType<KeybindFeature> {
    public KeybindFeature parse(StringReader stringReader) throws CommandSyntaxException {
        String readString = stringReader.readString();
        for (KeybindFeature keybindFeature : BaritoneHelper_3.searchHelper4_12.getRegistry()) {
            if (keybindFeature.getName().equalsIgnoreCase(readString)) {
                return keybindFeature;
            }
        }
        throw new DynamicCommandExceptionType(obj -> {
            return Text.literal(String.format("Macro %s doesn't exists", obj));
        }).create(readString);
    }

    public <S> CompletableFuture<Suggestions> listSuggestions(CommandContext<S> commandContext, SuggestionsBuilder suggestionsBuilder) {
        return CommandSource.suggestMatching(BaritoneHelper_3.searchHelper4_12.getRegistry().stream().map((v0) -> {
            return v0.getName();
        }), suggestionsBuilder);
    }
}
