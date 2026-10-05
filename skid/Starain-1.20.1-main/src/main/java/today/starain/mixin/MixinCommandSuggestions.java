package today.starain.mixin;

import com.mojang.brigadier.CommandDispatcher;
import com.mojang.brigadier.ParseResults;
import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.suggestion.Suggestions;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.components.CommandSuggestions;
import net.minecraft.client.gui.GuiGraphics;
import net.minecraft.client.gui.components.EditBox;
import net.minecraft.commands.SharedSuggestionProvider;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import today.starain.Starain;
import today.starain.utils.client.ChatHudRenderer;

import java.util.List;
import java.util.concurrent.CompletableFuture;

@Mixin(CommandSuggestions.class)
public abstract class MixinCommandSuggestions {
    @Shadow
    @Final
    EditBox input;

    @Shadow
    @Final
    Minecraft minecraft;

    @Shadow
    boolean keepSuggestions;

    @Shadow
    private ParseResults<SharedSuggestionProvider> currentParse;

    @Shadow
    private CompletableFuture<Suggestions> pendingSuggestions;

    @Shadow
    private CommandSuggestions.SuggestionsList suggestions;

    @Shadow
    private boolean allowSuggestions;

    @Shadow
    @Final
    private int fillColor;

    @Unique
    private boolean starain$suggestionShift;

    @Inject(method = "render", at = @At("HEAD"))
    private void starain$suggestionsHead(GuiGraphics guiGraphics, int mouseX, int mouseY, CallbackInfo ci) {
        if (!ChatHudRenderer.isActive()) {
            return;
        }
        ChatHudRenderer.beginSuggestions(fillColor);
        starain$suggestionShift = true;
        guiGraphics.pose().pushPose();
        guiGraphics.pose().translate(ChatHudRenderer.MARGIN, -ChatHudRenderer.MARGIN, 0.0f);
    }

    @Inject(method = "render", at = @At("RETURN"))
    private void starain$suggestionsReturn(GuiGraphics guiGraphics, int mouseX, int mouseY, CallbackInfo ci) {
        if (starain$suggestionShift) {
            starain$suggestionShift = false;
            guiGraphics.pose().popPose();
        }
    }

    @Redirect(
            method = "renderUsage",
            at = @At(value = "INVOKE", target = "Lnet/minecraft/client/gui/GuiGraphics;fill(IIIII)V")
    )
    private void starain$usageFill(GuiGraphics guiGraphics, int x1, int y1, int x2, int y2, int color) {
        if (!ChatHudRenderer.captureSuggestion(x1, y1, x2, y2, color)) {
            guiGraphics.fill(x1, y1, x2, y2, color);
        }
    }

    @Shadow
    @Final
    private boolean onlyShowIfCursorPastError;

    @Shadow
    @Final
    private List<?> commandUsage;

    @Shadow
    public abstract void showSuggestions(boolean narrateFirstSuggestion);

    @Shadow
    private void updateUsageInfo() {
    }

    @Inject(method = "updateCommandInfo", at = @At("HEAD"), cancellable = true)
    private void starain$updateDotCommandInfo(CallbackInfo ci) {
        String value = input.getValue();
        if (!value.startsWith(".")) {
            return;
        }

        Starain starain = Starain.getInstance();
        if (starain == null || starain.getCommandManager() == null
                || minecraft.player == null || minecraft.player.connection == null) {
            return;
        }

        if (currentParse != null && !currentParse.getReader().getString().equals(value)) {
            currentParse = null;
        }

        if (!keepSuggestions) {
            input.setSuggestion(null);
            suggestions = null;
        }

        commandUsage.clear();

        StringReader reader = new StringReader(value);
        reader.skip();

        int cursor = input.getCursorPosition();
        CommandDispatcher<SharedSuggestionProvider> dispatcher = starain.getCommandManager().getDispatcher();
        if (currentParse == null) {
            currentParse = dispatcher.parse(reader, minecraft.player.connection.getSuggestionsProvider());
        }

        int minimumCursor = onlyShowIfCursorPastError ? reader.getCursor() : 1;
        if (cursor >= minimumCursor && (suggestions == null || !keepSuggestions)) {
            pendingSuggestions = dispatcher.getCompletionSuggestions(currentParse, cursor);
            pendingSuggestions.thenRun(() -> {
                if (pendingSuggestions.isDone()) {
                    updateUsageInfo();
                    if (allowSuggestions) {
                        showSuggestions(false);
                    }
                }
            });
        }

        ci.cancel();
    }
}
