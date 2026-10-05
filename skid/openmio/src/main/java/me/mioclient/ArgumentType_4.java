package me.mioclient;

import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.util.Locale;
import java.util.concurrent.CompletableFuture;
import me.mioclient.api.Keybind;
import net.minecraft.client.util.InputUtil;
import net.minecraft.command.CommandSource;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ArgumentType_4.class */
public class ArgumentType_4 implements com.mojang.brigadier.arguments.ArgumentType<Keybind> {
    /* renamed from: parse, reason: merged with bridge method [inline-methods] */
    public Keybind parse(StringReader stringReader) throws com.mojang.brigadier.exceptions.CommandSyntaxException {
        String trim = stringReader.readString().toLowerCase(Locale.ROOT).trim();
        if (trim.startsWith("mouse")) {
            return new Keybind(Integer.parseInt(trim.toLowerCase().replace("mouse", "")), Keybind.KeybindMode.TOGGLE, true);
        }
        return new Keybind(InputUtil.fromTranslationKey(new ArgumentTypeHelper().getArgumentTypeHelper2919(trim.replace("_", ".")).getString2921("key.keyboard.\u0001")).getCode(), Keybind.KeybindMode.TOGGLE, false);
    }

    public <S> CompletableFuture<Suggestions> listSuggestions(CommandContext<S> commandContext, SuggestionsBuilder suggestionsBuilder) {
        ((me.mioclient.mixin.ducks.DuckInputUtilType) (Object) InputUtil.Type.KEYSYM).getKeyMap().forEach((num, key) -> {
            String upperCase = key.getTranslationKey().replaceAll("key.keyboard.", "").replace(".", "_").toUpperCase();
            if (CommandSource.shouldSuggest(suggestionsBuilder.getRemaining().toUpperCase(), upperCase)) {
                suggestionsBuilder.suggest(upperCase);
            }
        });
        for (int i = 0; i < 5; i++) {
            String string2921 = new ArgumentTypeHelper().getArgumentTypeHelper2906(i).getString2921("MOUSE\u0001");
            if (CommandSource.shouldSuggest(suggestionsBuilder.getRemaining(), string2921)) {
                suggestionsBuilder.suggest(string2921);
            }
        }
        return suggestionsBuilder.buildFuture();
    }
}
