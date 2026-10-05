package me.mioclient;

import java.io.DataOutput;
import org.jetbrains.annotations.NotNull;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/TooltipsDataOutput.class */
public class TooltipsDataOutput implements DataOutput {
    public int num;

    public int get3021() {
        return this.num;
    }

    @Override // java.io.DataOutput
    public void write(int i) {
        this.num++;
    }

    @Override // java.io.DataOutput
    public void write(byte[] bArr) {
        this.num += bArr.length;
    }

    @Override // java.io.DataOutput
    public void write(byte[] bArr, int i, int i2) {
        this.num += i2;
    }

    @Override // java.io.DataOutput
    public void writeBoolean(boolean z) {
        this.num++;
    }

    @Override // java.io.DataOutput
    public void writeByte(int i) {
        this.num++;
    }

    @Override // java.io.DataOutput
    public void writeShort(int i) {
        this.num += 2;
    }

    @Override // java.io.DataOutput
    public void writeChar(int i) {
        this.num += 2;
    }

    @Override // java.io.DataOutput
    public void writeInt(int i) {
        this.num += 4;
    }

    @Override // java.io.DataOutput
    public void writeLong(long j) {
        this.num += 8;
    }

    @Override // java.io.DataOutput
    public void writeFloat(float f) {
        this.num += 4;
    }

    @Override // java.io.DataOutput
    public void writeDouble(double d) {
        this.num += 8;
    }

    @Override // java.io.DataOutput
    public void writeBytes(String str) {
        this.num += str.length();
    }

    @Override // java.io.DataOutput
    public void writeChars(String str) {
        this.num += str.getBytes().length;
    }

    @Override // java.io.DataOutput
    public void writeUTF(@NotNull String str) {
        this.num += str.getBytes().length;
    }
}
