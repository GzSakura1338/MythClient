package com.jagrosh.discordipc.entities.pipe;

import com.google.gson.JsonObject;
import com.jagrosh.discordipc.IPCClient;
import com.jagrosh.discordipc.entities.Callback;
import com.jagrosh.discordipc.entities.Packet;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.HashMap;
import org.newsclub.net.unix.AFInputStream;
import org.newsclub.net.unix.AFUNIXSocket;
import org.newsclub.net.unix.AFUNIXSocketAddress;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:com/jagrosh/discordipc/entities/pipe/UnixPipe.class */
public class UnixPipe extends Pipe {
    public static final Logger LOGGER = LoggerFactory.getLogger(UnixPipe.class);
    public final AFUNIXSocket socket;

    public UnixPipe(IPCClient iPCClient, HashMap<String, Callback> hashMap, File file) throws java.io.IOException {
        super(iPCClient, hashMap);
        this.socket = AFUNIXSocket.newInstance();
        this.socket.connect(AFUNIXSocketAddress.of(file));
    }

    @Override // com.jagrosh.discordipc.entities.pipe.Pipe
    public Packet read() throws java.io.IOException {
        AFInputStream inputStream = this.socket.getInputStream();
        while (true) {
            if ((this.status == PipeStatus.CONNECTED || this.status == PipeStatus.CLOSING) && inputStream.available() == 0) {
                try {
                    Thread.sleep(50L);
                } catch (InterruptedException e) {
                }
            } else {
                break;
            }
        }
        if (this.status == PipeStatus.DISCONNECTED) {
            throw new IOException("Disconnected!");
        }
        if (this.status == PipeStatus.CLOSED) {
            return new Packet(Packet.OpCode.CLOSE, null, this.ipcClient.getEncoding());
        }
        byte[] bArr = new byte[8];
        int read = inputStream.read(bArr);
        ByteBuffer wrap = ByteBuffer.wrap(bArr);
        if (this.ipcClient.isDebugMode() && this.ipcClient.isVerboseLogging()) {
            this.ipcClient.getCurrentLogger(LOGGER).info(String.format("[DEBUG] Read Byte Data: %s with result %s", new String(bArr), Integer.valueOf(read)));
        }
        Packet.OpCode opCode = Packet.OpCode.values()[Integer.reverseBytes(wrap.getInt())];
        byte[] bArr2 = new byte[Integer.reverseBytes(wrap.getInt())];
        int read2 = inputStream.read(bArr2);
        if (this.ipcClient.isDebugMode() && this.ipcClient.isVerboseLogging()) {
            this.ipcClient.getCurrentLogger(LOGGER).info(String.format("[DEBUG] Read Reversed Byte Data: %s with result %s", new String(bArr2), Integer.valueOf(read2)));
        }
        return receive(opCode, bArr2);
    }

    @Override // com.jagrosh.discordipc.entities.pipe.Pipe
    public void write(byte[] bArr) throws java.io.IOException {
        this.socket.getOutputStream().write(bArr);
    }

    @Override // com.jagrosh.discordipc.entities.pipe.Pipe
    public void close() throws java.io.IOException {
        if (this.ipcClient.isDebugMode()) {
            this.ipcClient.getCurrentLogger(LOGGER).info("[DEBUG] Closing IPC pipe...");
        }
        this.status = PipeStatus.CLOSING;
        send(Packet.OpCode.CLOSE, new JsonObject());
        this.status = PipeStatus.CLOSED;
        this.socket.close();
    }

    public boolean mkdir(String str) {
        File file = new File(str);
        return (file.exists() && file.isDirectory()) || file.mkdir();
    }

    @Override // com.jagrosh.discordipc.entities.pipe.Pipe
    public void registerApp(String str, String str2) {
        String str3 = System.getenv("HOME");
        if (str3 == null) {
            throw new RuntimeException("Unable to find user HOME directory");
        }
        if (str2 == null) {
            try {
                str2 = Files.readSymbolicLink(Paths.get("/proc/self/exe", new String[0])).toString();
            } catch (Exception e) {
                throw new RuntimeException("Unable to get current exe path from /proc/self/exe", e);
            }
        }
        String str4 = "[Desktop Entry]\nName=Game " + str + "\nExec=" + str2 + " %%u\nType=Application\nNoDisplay=true\nCategories=Discord;Games;\nMimeType=x-scheme-handler/discord-" + str + ";\n";
        String str5 = "/discord-" + str + ".desktop";
        String str6 = str3 + "/.local";
        if (mkdir(str6)) {
            this.ipcClient.getCurrentLogger(LOGGER).warn("[DEBUG] Failed to create directory '" + str6 + "', may already exist");
        }
        String str7 = str6 + "/share";
        if (mkdir(str7)) {
            this.ipcClient.getCurrentLogger(LOGGER).warn("[DEBUG] Failed to create directory '" + str7 + "', may already exist");
        }
        String str8 = str7 + "/applications";
        if (mkdir(str8)) {
            this.ipcClient.getCurrentLogger(LOGGER).warn("[DEBUG] Failed to create directory '" + str8 + "', may already exist");
        }
        String str9 = str8 + str5;
        try {
            FileWriter fileWriter = new FileWriter(str9);
            try {
                fileWriter.write(str4);
                fileWriter.close();
                try {
                    ProcessBuilder processBuilder = new ProcessBuilder(("xdg-mime default discord-" + str + ".desktop x-scheme-handler/discord-" + str).split(" "));
                    processBuilder.environment();
                    int waitFor = processBuilder.start().waitFor();
                    if (waitFor < 0) {
                        throw new Exception("xdg-mime returned " + waitFor);
                    }
                } catch (Exception e2) {
                    throw new RuntimeException("Failed to register mime handler", e2);
                }
            } finally {
            }
        } catch (Exception e3) {
            throw new RuntimeException("Failed to write desktop info into '" + str9 + "'");
        }
    }

    @Override // com.jagrosh.discordipc.entities.pipe.Pipe
    public void registerSteamGame(String str, String str2) {
        registerApp(str, "xdg-open steam://rungameid/" + str2);
    }
}
