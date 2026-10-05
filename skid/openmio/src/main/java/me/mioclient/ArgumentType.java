package me.mioclient;

import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.exceptions.CommandSyntaxException;
import com.mojang.brigadier.exceptions.DynamicCommandExceptionType;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import me.mioclient.api.Setting;
import me.mioclient.module.Module;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ArgumentType.class */
public class ArgumentType implements com.mojang.brigadier.arguments.ArgumentType<String>, SearchHelper_4 {
    public final Module module;

    public ArgumentType(Module module) {
        this.module = module;
    }

    /* renamed from: parse, reason: merged with bridge method [inline-methods] */
    public String parse(StringReader stringReader) throws CommandSyntaxException {
        return stringReader.readString();
    }

    public <S> CompletableFuture<Suggestions> listSuggestions(CommandContext<S> commandContext, SuggestionsBuilder suggestionsBuilder) {
        return CommandSource.suggestMatching(this.module.getRegistry().stream().filter(setting -> {
            return !setting.is2352();
        }).map((v0) -> {
            return v0.getConfigName();
        }), suggestionsBuilder);
    }

    public static Setting<?> getOption(CommandContext<?> commandContext, Module module, String str) throws com.mojang.brigadier.exceptions.CommandSyntaxException {
        String str2 = (String) commandContext.getArgument(str, String.class);
        Optional<Setting<?>> findFirst = module.getRegistry().stream().filter(setting -> {
            return setting.getConfigName().equalsIgnoreCase(str2);
        }).findFirst();
        if (findFirst.isEmpty()) {
            throw new DynamicCommandExceptionType(obj -> {
                return Text.literal(new ArgumentTypeHelper().getArgumentTypeHelper2919(String.valueOf(obj)).getString2921("Setting not found \u0001"));
            }).create(str2);
        }
        return findFirst.get();
    }
}
