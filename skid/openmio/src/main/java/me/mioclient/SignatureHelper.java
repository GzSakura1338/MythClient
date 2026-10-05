package me.mioclient;

import me.mioclient.feature.Progress;
import net.minecraft.network.message.MessageSignatureData;
import net.minecraft.text.OrderedText;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SignatureHelper.class */
public interface SignatureHelper {
    MessageSignatureData getSignature();

    void setSignature(MessageSignatureData messageSignatureData);

    OrderedText getContent();

    void setContent(OrderedText orderedText);

    Progress getProgress();

    long mio$getAddTime();

    void mio$setAddTime(long j);
}
