package me.mioclient.mixin;

import com.mojang.brigadier.CommandDispatcher;
import com.mojang.brigadier.ParseResults;
import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.suggestion.Suggestions;
import java.util.concurrent.CompletableFuture;
import me.mioclient.ChatFilterSearchHelper4_2;
import me.mioclient.SearchHelper_4;
import net.minecraft.client.gui.screen.ChatInputSuggestor;
import net.minecraft.client.gui.widget.TextFieldWidget;
import net.minecraft.command.CommandSource;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.LocalCapture;

/* compiled from: 0.java */
@Mixin({ChatInputSuggestor.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinChatInputSuggester.class */
public abstract class MixinChatInputSuggester implements SearchHelper_4 {

    @Shadow
    @Final
    TextFieldWidget field_21599;

    @Shadow
    boolean field_21614;

    @Shadow
    private ParseResults<CommandSource> field_21610;

    @Shadow
    private CompletableFuture<Suggestions> field_21611;

    @Shadow
    private ChatInputSuggestor.SuggestionWindow field_21612;

    @Shadow
    public abstract void method_23920(boolean z);

    @Inject(method = {"refresh"}, at = {@At(value = "INVOKE", target = "Lcom/mojang/brigadier/StringReader;canRead()Z", remap = false)}, cancellable = true, locals = LocalCapture.CAPTURE_FAILHARD)
    public void refresh(CallbackInfo callbackInfo, String str, StringReader stringReader) {
        String string2982 = ChatFilterSearchHelper4_2.getString2982();
        if (stringReader.canRead(string2982.length()) && stringReader.getString().startsWith(string2982, stringReader.getCursor())) {
            stringReader.setCursor(stringReader.getCursor() + string2982.length());
            CommandDispatcher<CommandSource> commandDispatcher = ChatFilterSearchHelper4_2.commandDispatcher;
            if (this.field_21610 == null) {
                this.field_21610 = commandDispatcher.parse(stringReader, ChatFilterSearchHelper4_2.commandSource);
            }
            int cursor = this.field_21599.getCursor();
            if (cursor >= 1 && (this.field_21612 == null || !this.field_21614)) {
                this.field_21611 = commandDispatcher.getCompletionSuggestions(this.field_21610, cursor);
                this.field_21611.thenRun(() -> {
                    if (this.field_21611.isDone()) {
                        method_23920(false);
                    }
                });
            }
            callbackInfo.cancel();
        }
    }
}
