package com.jagrosh.discordipc.entities.pipe;

import com.google.gson.JsonObject;
import com.google.gson.JsonParseException;
import com.google.gson.JsonParser;
import com.jagrosh.discordipc.IPCClient;
import com.jagrosh.discordipc.IPCListener;
import com.jagrosh.discordipc.entities.Callback;
import com.jagrosh.discordipc.entities.DiscordBuild;
import com.jagrosh.discordipc.entities.Packet;
import com.jagrosh.discordipc.entities.User;
import com.jagrosh.discordipc.exceptions.NoDiscordClientException;
import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.UUID;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:com/jagrosh/discordipc/entities/pipe/Pipe.class */
public abstract class Pipe {
    public static final int VERSION = 1;
    public final IPCClient ipcClient;
    public final HashMap<String, Callback> callbacks;
    public PipeStatus status = PipeStatus.CONNECTING;
    public IPCListener listener;
    public DiscordBuild build;
    public User currentUser;
    public static final Logger LOGGER = LoggerFactory.getLogger(Pipe.class);
    public static final String[] unixPaths = {"XDG_RUNTIME_DIR", "TMPDIR", "TMP", "TEMP"};
    public static final String[] unixFolderPaths = {"/snap.discord", "/app/com.discordapp.Discord"};

    public Pipe(IPCClient iPCClient, HashMap<String, Callback> hashMap) {
        this.ipcClient = iPCClient;
        this.callbacks = hashMap;
    }

    public static Pipe openPipe(IPCClient iPCClient, long j, HashMap<String, Callback> hashMap, DiscordBuild... discordBuildArr) throws com.jagrosh.discordipc.exceptions.NoDiscordClientException {
        if (discordBuildArr == null || discordBuildArr.length == 0) {
            discordBuildArr = new DiscordBuild[]{DiscordBuild.ANY};
        }
        Pipe pipe = null;
        Pipe[] pipeArr = new Pipe[DiscordBuild.values().length];
        for (int i = 0; i < 10; i++) {
            String pipeLocation = getPipeLocation(i);
            if (iPCClient.isDebugMode()) {
                iPCClient.getCurrentLogger(LOGGER).info(String.format("[DEBUG] Searching for IPC Pipe: \"%s\"", pipeLocation));
            }
            try {
                File file = new File(pipeLocation);
                if (file.exists()) {
                    if (iPCClient.isDebugMode()) {
                        iPCClient.getCurrentLogger(LOGGER).info(String.format("[DEBUG] Found valid file, attempting connection to IPC: \"%s\"", pipeLocation));
                    }
                    pipe = createPipe(iPCClient, hashMap, file);
                    if (pipe != null) {
                        JsonObject jsonObject = new JsonObject();
                        jsonObject.addProperty("v", 1);
                        jsonObject.addProperty("client_id", Long.toString(j));
                        pipe.send(Packet.OpCode.HANDSHAKE, jsonObject);
                        Packet read = pipe.read();
                        JsonObject asJsonObject = read.getJson().getAsJsonObject("data");
                        JsonObject asJsonObject2 = asJsonObject.getAsJsonObject("user");
                        pipe.build = DiscordBuild.from(asJsonObject.getAsJsonObject("config").get("api_endpoint").getAsString());
                        pipe.currentUser = new User(asJsonObject2.getAsJsonPrimitive("username").getAsString(), (asJsonObject2.has("global_name") && asJsonObject2.get("global_name").isJsonPrimitive()) ? asJsonObject2.getAsJsonPrimitive("global_name").getAsString() : null, (asJsonObject2.has("discriminator") && asJsonObject2.get("discriminator").isJsonPrimitive()) ? asJsonObject2.getAsJsonPrimitive("discriminator").getAsString() : "0", Long.parseLong(asJsonObject2.getAsJsonPrimitive("id").getAsString()), (asJsonObject2.has("avatar") && asJsonObject2.get("avatar").isJsonPrimitive()) ? asJsonObject2.getAsJsonPrimitive("avatar").getAsString() : null);
                        if (iPCClient.isDebugMode()) {
                            iPCClient.getCurrentLogger(LOGGER).info(String.format("[DEBUG] Found a valid client (%s) with packet: %s", pipe.build.name(), read));
                            iPCClient.getCurrentLogger(LOGGER).info(String.format("[DEBUG] Found a valid user (%s) with id: %s", pipe.currentUser.getName(), pipe.currentUser.getId()));
                        }
                        if (pipe.build == discordBuildArr[0] || DiscordBuild.ANY == discordBuildArr[0]) {
                            if (iPCClient.isDebugMode()) {
                                iPCClient.getCurrentLogger(LOGGER).info(String.format("[DEBUG] Found preferred client: %s", pipe.build.name()));
                            }
                            break;
                        }
                        pipeArr[pipe.build.ordinal()] = pipe;
                        pipeArr[DiscordBuild.ANY.ordinal()] = pipe;
                        pipe.build = null;
                        pipe = null;
                    }
                } else if (iPCClient.isDebugMode()) {
                    iPCClient.getCurrentLogger(LOGGER).info(String.format("[DEBUG] Unable to locate IPC Pipe: \"%s\"", pipeLocation));
                }
            } catch (Exception e) {
                pipe = null;
            }
        }
        if (pipe == null) {
            int i2 = 1;
            while (true) {
                if (i2 >= discordBuildArr.length) {
                    break;
                }
                DiscordBuild discordBuild = discordBuildArr[i2];
                if (iPCClient.isDebugMode()) {
                    iPCClient.getCurrentLogger(LOGGER).info(String.format("[DEBUG] Looking for client build: %s", discordBuild.name()));
                }
                if (pipeArr[discordBuild.ordinal()] != null) {
                    pipe = pipeArr[discordBuild.ordinal()];
                    pipeArr[discordBuild.ordinal()] = null;
                    if (discordBuild == DiscordBuild.ANY) {
                        for (int i3 = 0; i3 < pipeArr.length; i3++) {
                            if (pipeArr[i3] == pipe) {
                                pipe.build = DiscordBuild.values()[i3];
                                pipeArr[i3] = null;
                            }
                        }
                    } else {
                        pipe.build = discordBuild;
                    }
                    if (iPCClient.isDebugMode()) {
                        iPCClient.getCurrentLogger(LOGGER).info(String.format("[DEBUG] Found preferred client: %s", pipe.build.name()));
                    }
                } else {
                    i2++;
                }
            }
            if (pipe == null) {
                throw new NoDiscordClientException();
            }
        }
        for (int i4 = 0; i4 < pipeArr.length; i4++) {
            if (i4 != DiscordBuild.ANY.ordinal() && pipeArr[i4] != null) {
                try {
                    pipeArr[i4].close();
                } catch (Exception e2) {
                    if (iPCClient.isDebugMode()) {
                        iPCClient.getCurrentLogger(LOGGER).info(String.format("[DEBUG] Failed to close an open IPC pipe: %s", e2));
                    }
                }
            }
        }
        pipe.status = PipeStatus.CONNECTED;
        return pipe;
    }

    public static Pipe createPipe(IPCClient iPCClient, HashMap<String, Callback> hashMap, File file) {
        String lowerCase = System.getProperty("os.name").toLowerCase();
        if (lowerCase.contains("win")) {
            return new WindowsPipe(iPCClient, hashMap, file);
        }
        if (!lowerCase.contains("linux") && !lowerCase.contains("mac")) {
            throw new RuntimeException("Unsupported OS: " + lowerCase);
        }
        try {
            return lowerCase.contains("mac") ? new MacPipe(iPCClient, hashMap, file) : new UnixPipe(iPCClient, hashMap, file);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    public static String generateNonce() {
        return UUID.randomUUID().toString();
    }

    public static String getPipeLocation(int i) {
        String str = null;
        String str2 = "discord-ipc-" + i;
        if (System.getProperty("os.name").contains("Win")) {
            return "\\\\?\\pipe\\" + str2;
        }
        for (String str3 : unixPaths) {
            str = System.getenv(str3);
            if (str != null) {
                break;
            }
        }
        if (str == null) {
            str = "/tmp";
        }
        String[] strArr = unixFolderPaths;
        int length = strArr.length;
        int i2 = 0;
        while (true) {
            if (i2 >= length) {
                break;
            }
            String str4 = str + strArr[i2];
            File file = new File(str4);
            if (file.exists() && file.isDirectory() && file.list().length > 0) {
                str = str4;
                break;
            }
            i2++;
        }
        return str + "/" + str2;
    }

    public void send(Packet.OpCode opCode, JsonObject jsonObject, Callback callback) {
        try {
            String generateNonce = generateNonce();
            jsonObject.addProperty("nonce", generateNonce);
            Packet packet = new Packet(opCode, jsonObject, this.ipcClient.getEncoding());
            if (callback != null && !callback.isEmpty()) {
                this.callbacks.put(generateNonce, callback);
            }
            write(packet.toBytes());
            if (this.ipcClient.isDebugMode()) {
                this.ipcClient.getCurrentLogger(LOGGER).info(String.format("[DEBUG] Sent packet: %s", packet.toDecodedString()));
            }
            if (this.listener != null) {
                this.listener.onPacketSent(this.ipcClient, packet);
            }
        } catch (Exception e) {
            this.ipcClient.getCurrentLogger(LOGGER).error("Encountered an IOException while sending a packet and disconnected!");
            this.status = PipeStatus.DISCONNECTED;
        }
    }

    public void send(Packet.OpCode opCode, JsonObject jsonObject) {
        send(opCode, jsonObject, null);
    }

    public Packet receive(Packet.OpCode opCode, byte[] bArr) {
        Packet packet = new Packet(opCode, new JsonParser().parse(new String(bArr)).getAsJsonObject(), this.ipcClient.getEncoding());
        if (this.ipcClient.isDebugMode()) {
            this.ipcClient.getCurrentLogger(LOGGER).info(String.format("[DEBUG] Received packet: %s", packet));
        }
        if (this.listener != null) {
            this.listener.onPacketReceived(this.ipcClient, packet);
        }
        return packet;
    }

    public abstract Packet read() throws java.io.IOException;

    public abstract void write(byte[] bArr) throws java.io.IOException;

    public abstract void registerApp(String str, String str2);

    public abstract void registerSteamGame(String str, String str2);

    public PipeStatus getStatus() {
        return this.status;
    }

    public void setStatus(PipeStatus pipeStatus) {
        this.status = pipeStatus;
    }

    public void setListener(IPCListener iPCListener) {
        this.listener = iPCListener;
    }

    public abstract void close() throws java.io.IOException;

    public DiscordBuild getDiscordBuild() {
        return this.build;
    }

    public User getCurrentUser() {
        return this.currentUser;
    }
}
