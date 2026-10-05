package com.jagrosh.discordipc.entities.pipe;

import com.google.gson.JsonObject;
import com.jagrosh.discordipc.IPCClient;
import com.jagrosh.discordipc.entities.Callback;
import com.jagrosh.discordipc.entities.Packet;
import com.jagrosh.discordipc.impl.WinRegistry;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.HashMap;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:com/jagrosh/discordipc/entities/pipe/WindowsPipe.class */
public class WindowsPipe extends Pipe {
    public static final Logger LOGGER = LoggerFactory.getLogger(WindowsPipe.class);
    public static final Float javaSpec = Float.valueOf(Float.parseFloat(System.getProperty("java.specification.version")));
    public int targetKey = -2147483647;
    public long targetLongKey = -2147483647L;
    public RandomAccessFile file;

    public WindowsPipe(IPCClient iPCClient, HashMap<String, Callback> hashMap, File file) {
        super(iPCClient, hashMap);
        this.targetKey = WinRegistry.HKEY_CURRENT_USER;
        this.targetLongKey = -2147483647L;
        try {
            this.file = new RandomAccessFile(file, "rw");
        } catch (FileNotFoundException e) {
            this.file = null;
        }
    }

    @Override // com.jagrosh.discordipc.entities.pipe.Pipe
    public void write(byte[] bArr) throws java.io.IOException {
        this.file.write(bArr);
    }

    @Override // com.jagrosh.discordipc.entities.pipe.Pipe
    public Packet read() throws java.io.IOException {
        while ((this.status == PipeStatus.CONNECTED || this.status == PipeStatus.CLOSING) && this.file.length() == 0) {
            try {
                Thread.sleep(50L);
            } catch (InterruptedException e) {
            }
        }
        if (this.status == PipeStatus.DISCONNECTED) {
            throw new IOException("Disconnected!");
        }
        if (this.status == PipeStatus.CLOSED) {
            return new Packet(Packet.OpCode.CLOSE, null, this.ipcClient.getEncoding());
        }
        Packet.OpCode opCode = Packet.OpCode.values()[Integer.reverseBytes(this.file.readInt())];
        byte[] bArr = new byte[Integer.reverseBytes(this.file.readInt())];
        this.file.readFully(bArr);
        return receive(opCode, bArr);
    }

    @Override // com.jagrosh.discordipc.entities.pipe.Pipe
    public void close() throws java.io.IOException {
        if (this.ipcClient.isDebugMode()) {
            this.ipcClient.getCurrentLogger(LOGGER).info("[DEBUG] Closing IPC pipe...");
        }
        this.status = PipeStatus.CLOSING;
        send(Packet.OpCode.CLOSE, new JsonObject());
        this.status = PipeStatus.CLOSED;
        this.file.close();
    }

    @Override // com.jagrosh.discordipc.entities.pipe.Pipe
    public void registerApp(String str, String str2) {
        String property = System.getProperty("java.home");
        File file = new File(property.split(";")[0] + "/bin/java.exe");
        File file2 = new File(property.split(";")[0] + "/bin/javaw.exe");
        String absolutePath = file.exists() ? file.getAbsolutePath() : file2.exists() ? file2.getAbsolutePath() : null;
        if (absolutePath == null) {
            throw new RuntimeException("Unable to find java path");
        }
        String str3 = str2 != null ? str2 : absolutePath;
        String str4 = "URL:Run game " + str + " protocol";
        String str5 = "Software\\Classes\\" + ("discord-" + str);
        String str6 = str5 + "\\DefaultIcon";
        String str7 = str5 + "\\DefaultIcon";
        try {
            if (javaSpec.floatValue() >= 11.0f) {
                WinRegistry.createKey(-2147483647L, str5);
                WinRegistry.writeStringValue(-2147483647L, str5, "", str4);
                WinRegistry.writeStringValue(-2147483647L, str5, "URL Protocol", "��");
                WinRegistry.createKey(-2147483647L, str6);
                WinRegistry.writeStringValue(-2147483647L, str6, "", absolutePath);
                WinRegistry.createKey(-2147483647L, str7);
                WinRegistry.writeStringValue(-2147483647L, str7, "", str3);
            } else {
                WinRegistry.createKey(WinRegistry.HKEY_CURRENT_USER, str5);
                WinRegistry.writeStringValue(WinRegistry.HKEY_CURRENT_USER, str5, "", str4);
                WinRegistry.writeStringValue(WinRegistry.HKEY_CURRENT_USER, str5, "URL Protocol", "��");
                WinRegistry.createKey(WinRegistry.HKEY_CURRENT_USER, str6);
                WinRegistry.writeStringValue(WinRegistry.HKEY_CURRENT_USER, str6, "", absolutePath);
                WinRegistry.createKey(WinRegistry.HKEY_CURRENT_USER, str7);
                WinRegistry.writeStringValue(WinRegistry.HKEY_CURRENT_USER, str7, "", str3);
            }
        } catch (Error | Exception e) {
            throw new RuntimeException("Unable to modify Discord registry keys", e);
        }
    }

    @Override // com.jagrosh.discordipc.entities.pipe.Pipe
    public void registerSteamGame(String str, String str2) {
        try {
            String readString = javaSpec.floatValue() >= 11.0f ? WinRegistry.readString(-2147483647L, "Software\\\\Valve\\\\Steam", "SteamExe") : WinRegistry.readString(WinRegistry.HKEY_CURRENT_USER, "Software\\\\Valve\\\\Steam", "SteamExe");
            if (readString == null) {
                throw new RuntimeException("Steam exe path not found");
            }
            registerApp(str, "\"" + readString.replaceAll("/", "\\") + "\" steam://rungameid/" + str2);
        } catch (Exception e) {
            throw new RuntimeException("Unable to register Steam game", e);
        }
    }
}
