package me.mioclient;

import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.exceptions.DynamicCommandExceptionType;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.util.concurrent.CompletableFuture;
import java.util.function.BiPredicate;
import net.minecraft.command.CommandSource;
import net.minecraft.registry.Registry;
import net.minecraft.text.Text;
import net.minecraft.util.Identifier;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/BiPredicateArgumentType.class */
public class BiPredicateArgumentType<T> implements com.mojang.brigadier.arguments.ArgumentType<T>, SearchHelper_4 {
    public final BiPredicate<CommandContext<?>, T> biPredicate;
    public final Registry<T> registry;

    public BiPredicateArgumentType(Registry<T> registry, BiPredicate<CommandContext<?>, T> biPredicate) {
        this.biPredicate = biPredicate;
        this.registry = registry;
    }

    public T parse(StringReader stringReader) throws com.mojang.brigadier.exceptions.CommandSyntaxException {
        String readString = stringReader.readString();
        return (T) this.registry.getOrEmpty(Identifier.of(readString)).orElseThrow(() -> {
            return new DynamicCommandExceptionType(obj -> {
                return Text.of(new ArgumentTypeHelper().getArgumentTypeHelper2919(String.valueOf(obj)).getString2921("Element not found \u0001"));
            }).create(readString);
        });
    }

    public <S> CompletableFuture<Suggestions> listSuggestions(CommandContext<S> commandContext, SuggestionsBuilder suggestionsBuilder) {
        return CommandSource.suggestMatching(this.registry.getIds().stream().filter(identifier -> {
            return test(commandContext, identifier);
        }).map((v0) -> {
            return v0.toShortTranslationKey();
        }), suggestionsBuilder);
    }

    public boolean test(CommandContext<?> commandContext, Identifier identifier) {
        return this.biPredicate.test(commandContext, this.registry.get(identifier));
    }

    public static <T> BiPredicateArgumentType<T> registry(Registry<T> registry) {
        return new BiPredicateArgumentType<>(registry, (commandContext, obj) -> {
            return true;
        });
    }

    public static <T> BiPredicateArgumentType<T> registry(Registry<T> registry, BiPredicate<CommandContext<?>, T> biPredicate) {
        return new BiPredicateArgumentType<>(registry, biPredicate);
    }
}
