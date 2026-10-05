package me.mioclient.feature;

import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.arguments.ArgumentType;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.exceptions.CommandSyntaxException;
import com.mojang.brigadier.exceptions.DynamicCommandExceptionType;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.util.Arrays;
import java.util.Locale;
import java.util.concurrent.CompletableFuture;
import me.mioclient.EnumSettingHelper;
import net.minecraft.command.CommandSource;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/feature/Enum.class */
public class Enum<T extends java.lang.Enum<T>> implements ArgumentType<T> {
    public final Class<T> class_;
    public final String string;

    public Enum(Class<T> cls) {
        this(cls, "Enum");
    }

    public Enum(Class<T> cls, String str) {
        this.class_ = cls;
        this.string = str;
    }

    /* renamed from: parse, reason: merged with bridge method [inline-methods] */
    public T parse(StringReader stringReader) throws CommandSyntaxException {
        String readString = stringReader.readString();
        for (T t : this.class_.getEnumConstants()) {
            if (name(t).equalsIgnoreCase(readString)) {
                return t;
            }
        }
        throw new DynamicCommandExceptionType(obj -> {
            return Text.literal(String.format("%s %s doesn't exist", this.string, obj));
        }).create(readString);
    }

    public <S> CompletableFuture<Suggestions> listSuggestions(CommandContext<S> commandContext, SuggestionsBuilder suggestionsBuilder) {
        return CommandSource.suggestMatching(Arrays.stream(this.class_.getEnumConstants()).map(this::name), suggestionsBuilder);
    }

    public String name(T t) {
        if (t instanceof EnumSettingHelper) {
            return ((EnumSettingHelper) t).getName();
        }
        return t.name().toLowerCase(Locale.ROOT);
    }
}
