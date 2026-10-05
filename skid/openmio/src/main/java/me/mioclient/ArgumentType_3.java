package me.mioclient;

import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.arguments.DoubleArgumentType;
import com.mojang.brigadier.arguments.FloatArgumentType;
import com.mojang.brigadier.arguments.IntegerArgumentType;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.util.Arrays;
import java.util.Collection;
import java.util.concurrent.CompletableFuture;
import me.mioclient.api.Setting;
import me.mioclient.module.Module;
import net.minecraft.command.CommandSource;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ArgumentType_3.class */
public class ArgumentType_3 implements com.mojang.brigadier.arguments.ArgumentType<String>, SearchHelper_4 {
    public final String string;
    public final Module module;

    /* renamed from: parse, reason: merged with bridge method [inline-methods] */
    public String parse(StringReader stringReader) throws com.mojang.brigadier.exceptions.CommandSyntaxException {
        String remaining = stringReader.getRemaining();
        stringReader.setCursor(stringReader.getTotalLength());
        return remaining;
    }

    /* JADX WARN: Multi-variable type inference failed */
    public <S> CompletableFuture<Suggestions> listSuggestions(CommandContext<S> commandContext, SuggestionsBuilder suggestionsBuilder) {
        Helper helper56;
        try {
            Setting<?> option = ArgumentType.getOption(commandContext, this.module, this.string);
            if (option instanceof BooleanSetting) {
                return ExamplesArgumentType_2.bool((BooleanSetting) option).listSuggestions(commandContext, suggestionsBuilder);
            }
            if (option instanceof EnumSetting) {
                return CommandSource.suggestMatching(Arrays.stream((Enum[]) ((Enum) ((EnumSetting) option).getObject2324()).getDeclaringClass().getEnumConstants()).filter(r2 -> {
                    return !EnumSettingConverterHelper.is1630(r2);
                }).map(r22 -> {
                    return r22.name().toLowerCase();
                }), suggestionsBuilder);
            }
            if (option instanceof NumberSetting) {
                NumberSetting numberSetting = (NumberSetting) option;
                Number number = (Number) numberSetting.getValue();
                return suggestionsBuilder.getRemaining().isBlank() ? suggestionsBuilder.suggest(String.valueOf(numberSetting.getObject2324())).buildFuture() : number instanceof Float ? FloatArgumentType.floatArg(((Float) ((Number) numberSetting.getObject2325())).floatValue(), ((Float) ((Number) numberSetting.getObject2326())).floatValue()).listSuggestions(commandContext, suggestionsBuilder) : number instanceof Double ? DoubleArgumentType.doubleArg(((Double) ((Number) numberSetting.getObject2325())).doubleValue(), ((Double) ((Number) numberSetting.getObject2326())).doubleValue()).listSuggestions(commandContext, suggestionsBuilder) : IntegerArgumentType.integer(((Integer) ((Number) numberSetting.getObject2325())).intValue(), ((Integer) ((Number) numberSetting.getObject2326())).intValue()).listSuggestions(commandContext, suggestionsBuilder);
            }
            if (!(option instanceof ObjectSetting)) {
                if (!(option instanceof SearchIdentifierSetting)) {
                    return com.mojang.brigadier.arguments.ArgumentType.super.listSuggestions(commandContext, suggestionsBuilder);
                }
                return CommandSource.suggestMatching(BaritoneHelper_3.searchHelper4_11.getSet2969().stream().map(searchIdentifier -> {
                    return new ArgumentTypeHelper().getArgumentTypeHelper2919(searchIdentifier.getName()).getArgumentTypeHelper2919(searchIdentifier.getString1610()).getString2921("\u0001:\u0001");
                }), suggestionsBuilder);
            }
            Collection<String> collection3127 = ((ObjectSetting) option).getCollection3127();
            if ((option instanceof EntityListObjectSetting) && (helper56 = Helper.getHelper56(((EntityListObjectSetting) option).getRegistry2387())) != Helper.helper_24) {
                collection3127.addAll(helper56.getCollection54());
            }
            return CommandSource.suggestMatching(collection3127, suggestionsBuilder);
        } catch (Exception e) {
            return Suggestions.empty();
        }
    }

    public ArgumentType_3(Module module, String str) {
        this.string = str;
        this.module = module;
    }

    public static ArgumentType_3 value(Module module, String str) {
        return new ArgumentType_3(module, str);
    }
}
