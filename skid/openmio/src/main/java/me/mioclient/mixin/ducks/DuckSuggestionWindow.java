package me.mioclient.mixin.ducks;

import net.minecraft.client.gui.screen.ChatInputSuggestor;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({ChatInputSuggestor.SuggestionWindow.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckSuggestionWindow.class */
public interface DuckSuggestionWindow {
    @Accessor("completed")
    boolean isCompleted();

    @Accessor("selection")
    int getSelection();

    @Accessor("typedText")
    String getTypedText();
}
