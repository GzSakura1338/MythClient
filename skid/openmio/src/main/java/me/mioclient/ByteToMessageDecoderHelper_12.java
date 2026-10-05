package me.mioclient;

import io.netty.channel.ChannelHandlerContext;
import java.io.DataInputStream;
import me.mioclient.ByteToMessageDecoderHelper;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ByteToMessageDecoderHelper_12.class */
public class ByteToMessageDecoderHelper_12 implements ByteToMessageDecoderHelper {
    public String string;

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public void do48(ChannelHandlerContext channelHandlerContext) {
        MixinMessageIndicatorHelper.do344(Text.literal(this.string), MixinMessageIndicatorHelper.getMessageSignatureData337(this.string.hashCode()));
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    /* renamed from: getByteToMessageDecoderHelper_12962, reason: merged with bridge method [inline-methods] */
    public ByteToMessageDecoderHelper_12 getByteToMessageDecoderHelper52(byte[] bArr) throws java.io.IOException {
        DataInputStream dataInputStream2177 = getDataInputStream2177(bArr);
        ByteToMessageDecoderHelper_12 byteToMessageDecoderHelper_12 = new ByteToMessageDecoderHelper_12();
        byteToMessageDecoderHelper_12.string = dataInputStream2177.readUTF();
        return byteToMessageDecoderHelper_12;
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public byte[] getByteArray50() throws java.io.IOException {
        ByteToMessageDecoderHelper.DataOutputStream dataOutputStream2178 = getDataOutputStream2178();
        dataOutputStream2178.writeUTF(this.string);
        return dataOutputStream2178.getByteArray1843();
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public short get51() {
        return (short) 13;
    }
}
