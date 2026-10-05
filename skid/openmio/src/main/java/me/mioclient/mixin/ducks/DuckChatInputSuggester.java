package me.mioclient.mixin.ducks;

import com.mojang.brigadier.ParseResults;
import com.mojang.brigadier.suggestion.Suggestions;
import java.util.concurrent.CompletableFuture;
import net.minecraft.client.gui.screen.ChatInputSuggestor;
import net.minecraft.command.CommandSource;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({ChatInputSuggestor.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckChatInputSuggester.class */
public interface DuckChatInputSuggester {
    @Accessor("pendingSuggestions")
    CompletableFuture<Suggestions> getSuggestion();

    @Accessor("window")
    ChatInputSuggestor.SuggestionWindow getWindow();

    @Accessor("parse")
    ParseResults<CommandSource> getParse();
}
