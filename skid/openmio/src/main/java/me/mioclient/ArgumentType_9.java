package me.mioclient;

import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.exceptions.DynamicCommandExceptionType;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ArgumentType_9.class */
public class ArgumentType_9 implements com.mojang.brigadier.arguments.ArgumentType<String> {
    public static PresetSearchHelper4 getPreset(CommandContext<?> commandContext, String str, String str2) throws com.mojang.brigadier.exceptions.CommandSyntaxException {
        PresetHelperMode presetHelperMode = (PresetHelperMode) commandContext.getArgument(str, PresetHelperMode.class);
        String str3 = (String) commandContext.getArgument(str2, String.class);
        Optional optional2404 = BaritoneHelper_3.presetHelper.getPresetHelperSearchHelper4_273(presetHelperMode).getOptional2404(presetSearchHelper4 -> {
            return presetSearchHelper4.getName().equalsIgnoreCase(str3);
        });
        if (optional2404.isEmpty()) {
            throw new DynamicCommandExceptionType(obj -> {
                return Text.of(new ArgumentTypeHelper().getArgumentTypeHelper2919(String.valueOf(obj)).getString2921("Preset not found \u0001"));
            }).create(str3);
        }
        return (PresetSearchHelper4) optional2404.get();
    }

    /* renamed from: parse, reason: merged with bridge method [inline-methods] */
    public String parse(StringReader stringReader) throws com.mojang.brigadier.exceptions.CommandSyntaxException {
        return stringReader.readString();
    }

    public <S> CompletableFuture<Suggestions> listSuggestions(CommandContext<S> commandContext, SuggestionsBuilder suggestionsBuilder) {
        return CompletableFuture.supplyAsync(() -> {
            PresetHelperSearchHelper4_2 presetHelperSearchHelper4_273 = BaritoneHelper_3.presetHelper.getPresetHelperSearchHelper4_273((PresetHelperMode) commandContext.getArgument("category", PresetHelperMode.class));
            try {
                presetHelperSearchHelper4_273.do34();
            } catch (Throwable th) {
            }
            try {
                return (Suggestions) CommandSource.suggestMatching(presetHelperSearchHelper4_273.getRegistry().stream().map((v0) -> {
                    return v0.getName();
                }), suggestionsBuilder).get();
            } catch (Exception e) {
                return null;
            }
        }, SearchHelper_4.executorService);
    }
}
