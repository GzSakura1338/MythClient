package dev.myth.mixin;

import com.mojang.brigadier.ParseResults;
import com.mojang.brigadier.suggestion.Suggestions;
import dev.myth.Myth;
import dev.myth.command.CommandManager;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.components.CommandSuggestions;
import net.minecraft.client.gui.components.EditBox;
import net.minecraft.client.multiplayer.ClientSuggestionProvider;
import net.minecraft.util.FormattedCharSequence;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(CommandSuggestions.class)
public abstract class MixinCommandSuggestions {
    @Shadow @Final private EditBox input;
    @Shadow @Final private Minecraft minecraft;
    @Shadow @Final private List<FormattedCharSequence> commandUsage;
    @Shadow private boolean keepSuggestions;
    @Shadow private boolean allowSuggestions;
    @Shadow private ParseResults<ClientSuggestionProvider> currentParse;
    @Shadow private boolean currentParseIsCommand;
    @Shadow private boolean currentParseIsMessage;
    @Shadow private CompletableFuture<Suggestions> pendingSuggestions;
    @Shadow private CommandSuggestions.SuggestionsList suggestions;
    @Shadow public abstract void showSuggestions(boolean narrateFirstSuggestion);

    @Inject(method = "updateCommandInfo", at = @At("HEAD"), cancellable = true)
    private void myth$dotSuggestions(CallbackInfo ci) {
        String value = input.getValue();
        if (!value.startsWith(CommandManager.getPrefix()) || minecraft.player == null) {
            return;
        }
        ci.cancel();
        currentParse = null;
        currentParseIsCommand = false;
        currentParseIsMessage = true;
        commandUsage.clear();
        if (keepSuggestions) {
            return;
        }
        input.setSuggestion(null);
        suggestions = null;
        int cursor = input.getCursorPosition();
        if (cursor < CommandManager.getPrefix().length()) {
            pendingSuggestions = null;
            return;
        }
        CompletableFuture<Suggestions> pending = Myth.COMMAND_MANAGER.getSuggestions(
                value, cursor, minecraft.player.connection.getSuggestionsProvider());
        pendingSuggestions = pending;
        pending.thenAccept(result -> minecraft.execute(() -> {
            if (pendingSuggestions == pending && input.getValue().equals(value)
                    && input.getCursorPosition() == cursor && allowSuggestions
                    && minecraft.options.autoSuggestions().get()) {
                showSuggestions(false);
            }
        }));
    }
}
