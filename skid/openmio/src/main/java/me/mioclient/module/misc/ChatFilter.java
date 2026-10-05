package me.mioclient.module.misc;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.ChatFilterSearchHelper4_2;
import me.mioclient.Helper_7;
import me.mioclient.KeyPearlMode;
import me.mioclient.MixinMessageIndicatorHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.AddMessageEvent;
import me.mioclient.event.Listen;
import me.mioclient.feature.Event;
import me.mioclient.module.Module;
import net.minecraft.client.gui.hud.MessageIndicator;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/ChatFilter.class */
public class ChatFilter extends Module {
    public Setting<Boolean> caseSensitive;
    public Setting<Boolean> ignoreSelf;
    public String string;

    public ChatFilter() {
        super("ChatFilter", "Filters chat messages based on your filters.", Category.MISC, new String[0]);
        PhaseESPHelper.do1351(this);
        this.string = null;
    }

    @Override // me.mioclient.module.Module
    public void onEnable() {
        if (BaritoneHelper_3.chatFilterSearchHelper4.isEmpty()) {
            MixinMessageIndicatorHelper.do345(Text.of("You don't have any chat filters set. Use the \"%schatfilter add <id> <filter>\" command to add new filters.".formatted(ChatFilterSearchHelper4_2.getString2982())), MixinMessageIndicatorHelper.getMessageSignatureData339(this), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode3);
        }
        this.string = null;
    }

    @Listen(get219= Helper_7.num)
    public void onAddMessage(AddMessageEvent addMessageEvent) {
        String string;
        if (addMessageEvent.getKeyPearlMode1472() != KeyPearlMode.Pre || addMessageEvent.getText2279() == null) {
            return;
        }
        MessageIndicator messageIndicator2284 = addMessageEvent.getMessageIndicator2284();
        if ((messageIndicator2284 == null || messageIndicator2284 == MessageIndicator.system() || messageIndicator2284 == MessageIndicator.singlePlayer() || messageIndicator2284 == MessageIndicator.notSecure()) && (string = addMessageEvent.getText2279().getString()) != null) {
            if (this.ignoreSelf.getValue().booleanValue() && this.string != null && string.contains(this.string)) {
                this.string = null;
            } else {
                if (BaritoneHelper_3.chatFilterSearchHelper4.isEmpty() || !BaritoneHelper_3.chatFilterSearchHelper4.is2677(string)) {
                    return;
                }
                addMessageEvent.do1162();
            }
        }
    }

    @Listen
    public void onDabigbulletz(Event event) {
        if (event.is2403() || event.getString2649() == null || !this.ignoreSelf.getValue().booleanValue()) {
            return;
        }
        this.string = event.getString2649();
    }
}
