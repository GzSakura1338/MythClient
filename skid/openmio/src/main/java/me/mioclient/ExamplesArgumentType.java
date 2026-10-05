package me.mioclient;

import com.mojang.brigadier.ImmutableStringReader;
import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.concurrent.CompletableFuture;
import net.minecraft.client.MinecraftClient;
import net.minecraft.command.CommandSource;
import net.minecraft.command.argument.CoordinateArgument;
import net.minecraft.command.argument.Vec3ArgumentType;
import net.minecraft.server.command.CommandManager;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ExamplesArgumentType.class */
public class ExamplesArgumentType implements com.mojang.brigadier.arguments.ArgumentType<Vec3d> {
    public static final Collection<String> collection = Arrays.asList("0 0 0", "~ ~ ~", "^ ^ ^", "^1 ^ ^-5", "0.1 -0.5 .9", "~0.5 ~1 ~-5");

    /* renamed from: parse, reason: merged with bridge method [inline-methods] */
    public Vec3d parse(StringReader stringReader) throws com.mojang.brigadier.exceptions.CommandSyntaxException {
        int cursor = stringReader.getCursor();
        CoordinateArgument parse = CoordinateArgument.parse(stringReader);
        if (!stringReader.canRead() || stringReader.peek() != ' ') {
            stringReader.setCursor(cursor);
            throw Vec3ArgumentType.INCOMPLETE_EXCEPTION.createWithContext((ImmutableStringReader) stringReader);
        }
        stringReader.skip();
        CoordinateArgument parse2 = CoordinateArgument.parse(stringReader, false);
        if (!stringReader.canRead() || stringReader.peek() != ' ') {
            stringReader.setCursor(cursor);
            throw Vec3ArgumentType.INCOMPLETE_EXCEPTION.createWithContext((ImmutableStringReader) stringReader);
        }
        stringReader.skip();
        Vec3d pos = MinecraftClient.getInstance().player.getPos();
        CoordinateArgument parse3 = CoordinateArgument.parse(stringReader);
        return new Vec3d(parse.toAbsoluteCoordinate(pos.x), parse2.toAbsoluteCoordinate(pos.y), parse3.toAbsoluteCoordinate(pos.z));
    }

    public <S> CompletableFuture<Suggestions> listSuggestions(CommandContext<S> commandContext, SuggestionsBuilder suggestionsBuilder) {
        if (!(commandContext.getSource() instanceof CommandSource)) {
            return Suggestions.empty();
        }
        String remaining = suggestionsBuilder.getRemaining();
        return CommandSource.suggestPositions(remaining, (remaining.isEmpty() || remaining.charAt(0) != '^') ? ((CommandSource) commandContext.getSource()).getPositionSuggestions() : Collections.singleton(CommandSource.RelativePosition.ZERO_LOCAL), suggestionsBuilder, CommandManager.getCommandValidator(this::parse));
    }

    public Collection<String> getExamples() {
        return collection;
    }
}
