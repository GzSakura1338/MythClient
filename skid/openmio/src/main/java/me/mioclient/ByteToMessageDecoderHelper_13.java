package me.mioclient;

import io.netty.channel.ChannelHandlerContext;
import java.io.DataInputStream;
import me.mioclient.ByteToMessageDecoderHelper;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ByteToMessageDecoderHelper_13.class */
public class ByteToMessageDecoderHelper_13 implements ByteToMessageDecoderHelper {
    public String string;
    public int num;
    public String string2;
    public byte[] byteArr;

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public void do48(ChannelHandlerContext channelHandlerContext) {
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    /* renamed from: getByteToMessageDecoderHelper_13971, reason: merged with bridge method [inline-methods] */
    public ByteToMessageDecoderHelper_13 getByteToMessageDecoderHelper52(byte[] bArr) throws java.io.IOException {
        DataInputStream dataInputStream2177 = getDataInputStream2177(bArr);
        ByteToMessageDecoderHelper_13 byteToMessageDecoderHelper_13 = new ByteToMessageDecoderHelper_13();
        byteToMessageDecoderHelper_13.num = dataInputStream2177.readInt();
        byteToMessageDecoderHelper_13.string2 = dataInputStream2177.readUTF();
        byteToMessageDecoderHelper_13.string = dataInputStream2177.readUTF();
        int readInt = dataInputStream2177.readInt();
        if (readInt <= 0 || readInt > 1000) {
            byteToMessageDecoderHelper_13.byteArr = null;
        } else {
            byteToMessageDecoderHelper_13.byteArr = new byte[readInt];
            dataInputStream2177.readFully(byteToMessageDecoderHelper_13.byteArr);
        }
        return byteToMessageDecoderHelper_13;
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public byte[] getByteArray50() throws java.io.IOException {
        ByteToMessageDecoderHelper.DataOutputStream dataOutputStream2178 = getDataOutputStream2178();
        dataOutputStream2178.writeInt(this.num);
        dataOutputStream2178.writeUTF(this.string2);
        dataOutputStream2178.writeUTF(this.string);
        if (this.byteArr == null) {
            dataOutputStream2178.writeInt(0);
        } else {
            dataOutputStream2178.writeInt(this.byteArr.length);
            dataOutputStream2178.write(this.byteArr);
        }
        return dataOutputStream2178.getByteArray1843();
    }

    @Override // me.mioclient.ByteToMessageDecoderHelper
    public short get51() {
        return (short) 1;
    }

    public void do972(int i) {
        this.num = i;
    }

    public void do973(byte[] bArr) {
        this.byteArr = bArr;
    }

    public void do974(String str) {
        if (str == null) {
            this.string2 = "none";
        } else {
            this.string2 = str;
        }
    }

    public void do975(String str) {
        this.string = str;
    }
}
