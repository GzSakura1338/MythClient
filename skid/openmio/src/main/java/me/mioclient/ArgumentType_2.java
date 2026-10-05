package me.mioclient;

import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.exceptions.DynamicCommandExceptionType;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.util.concurrent.CompletableFuture;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ArgumentType_2.class */
public class ArgumentType_2 implements com.mojang.brigadier.arguments.ArgumentType<ChestStealerEnumSettingHelper> {
    /* renamed from: parse, reason: merged with bridge method [inline-methods] */
    public ChestStealerEnumSettingHelper parse(StringReader stringReader) throws com.mojang.brigadier.exceptions.CommandSyntaxException {
        String readString = stringReader.readString();
        for (ChestStealerEnumSettingHelper chestStealerEnumSettingHelper : BaritoneHelper_3.chestStealerSearchHelper4_3.getRegistry()) {
            if (chestStealerEnumSettingHelper.getName().equalsIgnoreCase(readString)) {
                return chestStealerEnumSettingHelper;
            }
        }
        throw new DynamicCommandExceptionType(obj -> {
            return Text.literal(String.format("Kit %s doesn't exist", obj));
        }).create(readString);
    }

    public <S> CompletableFuture<Suggestions> listSuggestions(CommandContext<S> commandContext, SuggestionsBuilder suggestionsBuilder) {
        return CommandSource.suggestMatching(BaritoneHelper_3.chestStealerSearchHelper4_3.getRegistry().stream().map((v0) -> {
            return v0.getName();
        }), suggestionsBuilder);
    }
}
