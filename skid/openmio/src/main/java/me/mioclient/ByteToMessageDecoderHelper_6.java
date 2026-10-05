package me.mioclient;

import io.netty.channel.ChannelHandlerContext;
import java.io.DataInputStream;
import me.mioclient.ByteToMessageDecoderHelper;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ByteToMessageDecoderHelper_6.class */
public class ByteToMessageDecoderHelper_6 implements ByteToMessageDecoderHelper {
    public String[] stringArr;
    public boolean flag;

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public void do48(ChannelHandlerContext channelHandlerContext) {
        BaritoneHelper_3.nameTagsSearchHelper4.do2311(this.stringArr);
        SearchHelper4_13.do0(channelHandlerContext.channel());
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    /* renamed from: getByteToMessageDecoderHelper_63066, reason: merged with bridge method [inline-methods] */
    public ByteToMessageDecoderHelper_6 getByteToMessageDecoderHelper52(byte[] bArr) throws java.io.IOException {
        DataInputStream dataInputStream2177 = getDataInputStream2177(bArr);
        ByteToMessageDecoderHelper_6 byteToMessageDecoderHelper_6 = new ByteToMessageDecoderHelper_6();
        byteToMessageDecoderHelper_6.flag = dataInputStream2177.readBoolean();
        int readInt = dataInputStream2177.readInt();
        byteToMessageDecoderHelper_6.stringArr = new String[readInt];
        for (int i = 0; i < readInt; i++) {
            byteToMessageDecoderHelper_6.stringArr[i] = dataInputStream2177.readUTF();
        }
        return byteToMessageDecoderHelper_6;
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public byte[] getByteArray50() throws java.io.IOException {
        ByteToMessageDecoderHelper.DataOutputStream dataOutputStream2178 = getDataOutputStream2178();
        dataOutputStream2178.writeBoolean(this.flag);
        dataOutputStream2178.writeInt(this.stringArr.length);
        for (String str : this.stringArr) {
            dataOutputStream2178.writeUTF(str);
        }
        return dataOutputStream2178.getByteArray1843();
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public short get51() {
        return (short) 12;
    }
}
