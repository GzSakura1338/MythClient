package me.mioclient.event;

import me.mioclient.KeyPearlMode;
import net.minecraft.client.gui.hud.ChatHudLine;
import net.minecraft.client.gui.hud.MessageIndicator;
import net.minecraft.network.message.MessageSignatureData;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/event/AddMessageEvent.class */
public class AddMessageEvent extends Event {
    public final KeyPearlMode keyPearlMode;
    public final MessageIndicator messageIndicator;
    public MessageSignatureData signature;
    public Text text;
    public ChatHudLine.Visible visible;
    public String string;

    public AddMessageEvent(MessageIndicator messageIndicator, MessageSignatureData messageSignatureData, Text text) {
        this.messageIndicator = messageIndicator;
        this.signature = messageSignatureData;
        this.text = text;
        this.keyPearlMode = KeyPearlMode.Pre;
        this.string = text == null ? null : text.getString();
    }

    public AddMessageEvent(ChatHudLine.Visible visible) {
        this.visible = visible;
        this.keyPearlMode = KeyPearlMode.Post;
        this.messageIndicator = null;
    }

    public MessageSignatureData getSignature() {
        return this.signature;
    }

    public void setSignature(MessageSignatureData messageSignatureData) {
        this.signature = messageSignatureData;
    }

    public Text getText2279() {
        return this.text;
    }

    public void do2280(Text text) {
        this.text = text;
    }

    public ChatHudLine.Visible getVisible2281() {
        return this.visible;
    }

    public void do2282(ChatHudLine.Visible visible) {
        this.visible = visible;
    }

    public KeyPearlMode getKeyPearlMode1472() {
        return this.keyPearlMode;
    }

    public String getString2283() {
        return this.string;
    }

    public MessageIndicator getMessageIndicator2284() {
        return this.messageIndicator;
    }
}
