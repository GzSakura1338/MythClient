package me.mioclient;

import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.exceptions.CommandSyntaxException;
import com.mojang.brigadier.exceptions.DynamicCommandExceptionType;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.util.Arrays;
import java.util.Locale;
import java.util.concurrent.CompletableFuture;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ArgumentType_6.class */
public class ArgumentType_6 implements com.mojang.brigadier.arguments.ArgumentType<PresetHelperMode> {
    public static PresetHelperSearchHelper4_2 getManager(CommandContext<?> commandContext, String str) {
        return BaritoneHelper_3.presetHelper.getPresetHelperSearchHelper4_273((PresetHelperMode) commandContext.getArgument(str, PresetHelperMode.class));
    }

    /* renamed from: parse, reason: merged with bridge method [inline-methods] */
    public PresetHelperMode parse(StringReader stringReader) throws CommandSyntaxException {
        String readString = stringReader.readString();
        PresetHelperMode presetHelperMode3013 = PresetHelperMode.getPresetHelperMode3013(readString);
        if (presetHelperMode3013 == null) {
            throw new DynamicCommandExceptionType(obj -> {
                return Text.of(new ArgumentTypeHelper().getArgumentTypeHelper2919(String.valueOf(obj)).getString2921("Preset category not found \u0001"));
            }).create(readString);
        }
        return presetHelperMode3013;
    }

    public <S> CompletableFuture<Suggestions> listSuggestions(CommandContext<S> commandContext, SuggestionsBuilder suggestionsBuilder) {
        return CommandSource.suggestMatching(Arrays.stream(PresetHelperMode.values()).map(presetHelperMode -> {
            return presetHelperMode.getName().toLowerCase(Locale.ROOT);
        }), suggestionsBuilder);
    }
}
