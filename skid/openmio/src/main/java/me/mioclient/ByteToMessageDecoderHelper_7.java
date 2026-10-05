package me.mioclient;

import io.netty.channel.ChannelHandlerContext;
import java.io.DataInputStream;
import me.mioclient.ByteToMessageDecoderHelper;
import net.minecraft.text.Text;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ByteToMessageDecoderHelper_7.class */
public class ByteToMessageDecoderHelper_7 implements ByteToMessageDecoderHelper {
    public String[] stringArr;

    public ByteToMessageDecoderHelper_7() {
    }

    public ByteToMessageDecoderHelper_7(String[] strArr) {
        this.stringArr = strArr;
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public void do48(ChannelHandlerContext channelHandlerContext) {
        String formatted = "%d online players: %s".formatted(Integer.valueOf(this.stringArr.length), String.join(", ", this.stringArr));
        MixinMessageIndicatorHelper.do344(Text.literal(formatted), MixinMessageIndicatorHelper.getMessageSignatureData337(formatted.hashCode()));
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    /* renamed from: getByteToMessageDecoderHelper_749, reason: merged with bridge method [inline-methods] */
    public ByteToMessageDecoderHelper_7 getByteToMessageDecoderHelper52(byte[] bArr) throws java.io.IOException {
        DataInputStream dataInputStream2177 = getDataInputStream2177(bArr);
        int readInt = dataInputStream2177.readInt();
        ByteToMessageDecoderHelper_7 byteToMessageDecoderHelper_7 = new ByteToMessageDecoderHelper_7();
        byteToMessageDecoderHelper_7.stringArr = new String[readInt];
        for (int i = 0; i < readInt; i++) {
            byteToMessageDecoderHelper_7.stringArr[i] = dataInputStream2177.readUTF();
        }
        return byteToMessageDecoderHelper_7;
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public byte[] getByteArray50() throws java.io.IOException {
        ByteToMessageDecoderHelper.DataOutputStream dataOutputStream2178 = getDataOutputStream2178();
        dataOutputStream2178.writeInt(this.stringArr.length);
        for (String str : this.stringArr) {
            dataOutputStream2178.writeUTF(str);
        }
        return dataOutputStream2178.getByteArray1843();
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public short get51() {
        return (short) 3;
    }
}
