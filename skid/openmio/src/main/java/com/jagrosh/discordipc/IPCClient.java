package com.jagrosh.discordipc;

import com.google.gson.JsonObject;
import com.google.gson.JsonParseException;
import com.jagrosh.discordipc.entities.Callback;
import com.jagrosh.discordipc.entities.DiscordBuild;
import com.jagrosh.discordipc.entities.Packet;
import com.jagrosh.discordipc.entities.RichPresence;
import com.jagrosh.discordipc.entities.User;
import com.jagrosh.discordipc.entities.pipe.Pipe;
import com.jagrosh.discordipc.entities.pipe.PipeStatus;
import com.jagrosh.discordipc.impl.Backoff;
import java.io.Closeable;
import java.io.IOException;
import java.lang.management.ManagementFactory;
import java.util.HashMap;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:com/jagrosh/discordipc/IPCClient.class */
public final class IPCClient implements Closeable {
    public static final Logger LOGGER = LoggerFactory.getLogger(IPCClient.class);
    public final Backoff RECONNECT_TIME_MS;
    public final long clientId;
    public final boolean autoRegister;
    public final HashMap<String, Callback> callbacks;
    public final String applicationId;
    public final String optionalSteamId;
    public volatile Pipe pipe;
    public Logger forcedLogger;
    public IPCListener listener;
    public Thread readThread;
    public String encoding;
    public long nextDelay;
    public boolean debugMode;
    public boolean verboseLogging;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:com/jagrosh/discordipc/IPCClient$ApprovalMode.class */
    public enum ApprovalMode {
        ACCEPT,
        DENY
    }

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:com/jagrosh/discordipc/IPCClient$Event.class */
    public enum Event {
        NULL(false),
        READY(false),
        ERROR(false),
        ACTIVITY_JOIN(true),
        ACTIVITY_SPECTATE(true),
        ACTIVITY_JOIN_REQUEST(true),
        UNKNOWN(false);

        public final boolean subscribable;

        Event(boolean z) {
            this.subscribable = z;
        }

        public static Event of(String str) {
            if (str == null) {
                return NULL;
            }
            for (Event event : values()) {
                if (event != UNKNOWN && event.name().equalsIgnoreCase(str)) {
                    return event;
                }
            }
            return UNKNOWN;
        }

        public boolean isSubscribable() {
            return this.subscribable;
        }
    }

    public IPCClient(long j, boolean z, boolean z2, boolean z3, String str, String str2) {
        this.RECONNECT_TIME_MS = new Backoff(500L, 60000L);
        this.callbacks = new HashMap<>();
        this.forcedLogger = null;
        this.listener = null;
        this.readThread = null;
        this.encoding = "UTF-8";
        this.nextDelay = 0L;
        this.clientId = j;
        this.debugMode = z;
        this.verboseLogging = z2;
        this.applicationId = str;
        this.autoRegister = z3;
        this.optionalSteamId = str2;
    }

    public IPCClient(long j, boolean z, boolean z2, boolean z3, String str) {
        this(j, z, z2, z3, str, null);
    }

    public IPCClient(long j, boolean z, boolean z2) {
        this(j, z, z2, false, (String) null);
    }

    public IPCClient(long j, boolean z, boolean z2, String str, String str2) {
        this(j, z, false, z2, str, str2);
    }

    public IPCClient(long j, boolean z, boolean z2, String str) {
        this(j, z, z2, str, (String) null);
    }

    public IPCClient(long j, boolean z) {
        this(j, z, false, (String) null);
    }

    public IPCClient(long j, boolean z, String str, String str2) {
        this(j, false, z, str, str2);
    }

    public IPCClient(long j, boolean z, String str) {
        this(j, z, str, (String) null);
    }

    public IPCClient(long j) {
        this(j, false, (String) null);
    }

    public static int getPID() {
        String name = ManagementFactory.getRuntimeMXBean().getName();
        return Integer.parseInt(name.substring(0, name.indexOf(64)));
    }

    public Logger getCurrentLogger(Logger logger) {
        return this.forcedLogger != null ? this.forcedLogger : logger;
    }

    public void setForcedLogger(Logger logger) {
        this.forcedLogger = logger;
    }

    public void setListener(IPCListener iPCListener) {
        this.listener = iPCListener;
        if (this.pipe != null) {
            this.pipe.setListener(iPCListener);
        }
    }

    public String getApplicationId() {
        return this.applicationId;
    }

    public String getOptionalSteamId() {
        return this.optionalSteamId;
    }

    public boolean isAutoRegister() {
        return this.autoRegister;
    }

    public String getEncoding() {
        return this.encoding;
    }

    public void setEncoding(String str) {
        this.encoding = str;
    }

    public long getClientID() {
        return this.clientId;
    }

    public boolean isDebugMode() {
        return this.debugMode;
    }

    public void setDebugMode(boolean z) {
        this.debugMode = z;
    }

    public boolean isVerboseLogging() {
        return this.verboseLogging;
    }

    public void setVerboseLogging(boolean z) {
        this.verboseLogging = z;
    }

    public void connect(DiscordBuild... discordBuildArr) throws com.jagrosh.discordipc.exceptions.NoDiscordClientException {
        checkConnected(false);
        while (true) {
            long currentTimeMillis = this.nextDelay - System.currentTimeMillis();
            if (currentTimeMillis <= 0) {
                break;
            }
            if (this.debugMode) {
                getCurrentLogger(LOGGER).info("[DEBUG] Attempting connection in: " + currentTimeMillis + "ms");
            }
            try { Thread.sleep(currentTimeMillis); } catch (InterruptedException ie) { Thread.currentThread().interrupt(); return; }
        }
        this.callbacks.clear();
        this.pipe = null;
        try {
            this.pipe = Pipe.openPipe(this, this.clientId, this.callbacks, discordBuildArr);
            if (isAutoRegister()) {
                try {
                    if (this.optionalSteamId == null || this.optionalSteamId.isEmpty()) {
                        registerApp(getApplicationId(), null);
                    } else {
                        registerSteamGame(getApplicationId(), this.optionalSteamId);
                    }
                } catch (Error | Exception e) {
                    if (this.debugMode) {
                        e.printStackTrace();
                    } else {
                        getCurrentLogger(LOGGER).error("Unable to register application, enable debug mode for trace...");
                    }
                }
            }
            if (this.debugMode) {
                getCurrentLogger(LOGGER).info("[DEBUG] Client is now connected and ready!");
            }
            if (this.listener != null) {
                this.listener.onReady(this);
                this.pipe.setListener(this.listener);
            }
            startReading();
        } catch (Exception e2) {
            updateReconnectTime();
            if (e2 instanceof com.jagrosh.discordipc.exceptions.NoDiscordClientException) throw (com.jagrosh.discordipc.exceptions.NoDiscordClientException) e2;
            throw new java.lang.RuntimeException(e2);
        }
    }

    public void sendRichPresence(RichPresence richPresence) {
        sendRichPresence(richPresence, null);
    }

    public void sendRichPresence(RichPresence richPresence, Callback callback) {
        checkConnected(true);
        if (this.debugMode) {
            getCurrentLogger(LOGGER).info("[DEBUG] Sending RichPresence to discord: " + (richPresence == null ? null : richPresence.toDecodedJson(this.encoding)));
        }
        JsonObject jsonObject = new JsonObject();
        JsonObject jsonObject2 = new JsonObject();
        jsonObject.addProperty("cmd", "SET_ACTIVITY");
        jsonObject2.addProperty("pid", Integer.valueOf(getPID()));
        jsonObject2.add("activity", richPresence == null ? new JsonObject() : richPresence.toJson());
        jsonObject.add("args", jsonObject2);
        this.pipe.send(Packet.OpCode.FRAME, jsonObject, callback);
    }

    public void registerSteamGame(String str, String str2) {
        if (this.pipe != null) {
            this.pipe.registerSteamGame(str, str2);
        }
    }

    public void registerApp(String str, String str2) {
        if (this.pipe != null) {
            this.pipe.registerApp(str, str2);
        }
    }

    public void subscribe(Event event) {
        subscribe(event, null);
    }

    public void subscribe(Event event, Callback callback) {
        checkConnected(true);
        if (!event.isSubscribable()) {
            throw new IllegalStateException("Cannot subscribe to " + event + " event!");
        }
        if (this.debugMode) {
            getCurrentLogger(LOGGER).info(String.format("[DEBUG] Subscribing to Event: %s", event.name()));
        }
        JsonObject jsonObject = new JsonObject();
        jsonObject.addProperty("cmd", "SUBSCRIBE");
        jsonObject.addProperty("evt", event.name());
        this.pipe.send(Packet.OpCode.FRAME, jsonObject, callback);
    }

    public void respondToJoinRequest(User user, ApprovalMode approvalMode, Callback callback) {
        checkConnected(true);
        if (user != null) {
            if (this.debugMode) {
                getCurrentLogger(LOGGER).info(String.format("[DEBUG] Sending response to %s as %s", user.getName(), approvalMode.name()));
            }
            JsonObject jsonObject = new JsonObject();
            jsonObject.addProperty("cmd", approvalMode == ApprovalMode.ACCEPT ? "SEND_ACTIVITY_JOIN_INVITE" : "CLOSE_ACTIVITY_JOIN_REQUEST");
            JsonObject jsonObject2 = new JsonObject();
            jsonObject2.addProperty("user_id", user.getId());
            jsonObject.add("args", jsonObject2);
            this.pipe.send(Packet.OpCode.FRAME, jsonObject, callback);
        }
    }

    public void respondToJoinRequest(User user, ApprovalMode approvalMode) {
        respondToJoinRequest(user, approvalMode, null);
    }

    public PipeStatus getStatus() {
        return this.pipe == null ? PipeStatus.UNINITIALIZED : this.pipe.getStatus();
    }

    @Override // java.io.Closeable, java.lang.AutoCloseable
    public void close() {
        checkConnected(true);
        try {
            this.pipe.close();
        } catch (Exception e) {
            if (this.debugMode) {
                getCurrentLogger(LOGGER).info(String.format("[DEBUG] Failed to close pipe: %s", e));
            }
        }
    }

    public DiscordBuild getDiscordBuild() {
        if (this.pipe == null) {
            return null;
        }
        return this.pipe.getDiscordBuild();
    }

    public User getCurrentUser() {
        if (this.pipe == null) {
            return null;
        }
        return this.pipe.getCurrentUser();
    }

    public void checkConnected(boolean z) {
        if (z && getStatus() != PipeStatus.CONNECTED) {
            throw new IllegalStateException(String.format("IPCClient (ID: %d) is not connected!", Long.valueOf(this.clientId)));
        }
        if (!z && getStatus() == PipeStatus.CONNECTED) {
            throw new IllegalStateException(String.format("IPCClient (ID: %d) is already connected!", Long.valueOf(this.clientId)));
        }
    }

    public void startReading() {
        this.readThread = new Thread(new Runnable() { // from class: com.jagrosh.discordipc.IPCClient.1
            @Override // java.lang.Runnable
            public void run() {
                IPCClient.this.readPipe(IPCClient.this);
            }
        }, "IPCClient-Reader");
        this.readThread.setDaemon(true);
        if (this.debugMode) {
            getCurrentLogger(LOGGER).info("[DEBUG] Starting IPCClient reading thread!");
        }
        this.readThread.start();
    }

    /* JADX WARN: Can't fix incorrect switch cases order, some code will duplicate */
    /* JADX WARN: Code restructure failed: missing block: B:47:0x0193, code lost:
    
        if (r10.listener == null) goto L101;
     */
    /* JADX WARN: Code restructure failed: missing block: B:49:0x019d, code lost:
    
        if (r0.has("cmd") == false) goto L102;
     */
    /* JADX WARN: Code restructure failed: missing block: B:51:0x01b0, code lost:
    
        if (r0.getAsJsonPrimitive("cmd").getAsString().equals("DISPATCH") == false) goto L103;
     */
    /* JADX WARN: Code restructure failed: missing block: B:53:0x01b3, code lost:
    
        r0 = r0.getAsJsonObject("data");
     */
    /* JADX WARN: Code restructure failed: missing block: B:54:0x01d0, code lost:
    
        switch(com.jagrosh.discordipc.IPCClient.AnonymousClass2.$SwitchMap$com$jagrosh$discordipc$IPCClient$Event[com.jagrosh.discordipc.IPCClient.Event.of(r0.getAsJsonPrimitive("evt").getAsString()).ordinal()]) {
            case 3: goto L56;
            case 4: goto L57;
            case 5: goto L58;
            default: goto L81;
        };
     */
    /* JADX WARN: Code restructure failed: missing block: B:55:0x01ec, code lost:
    
        r10.listener.onActivityJoin(r11, r0.getAsJsonPrimitive("secret").getAsString());
     */
    /* JADX WARN: Code restructure failed: missing block: B:57:0x0204, code lost:
    
        r10.listener.onActivitySpectate(r11, r0.getAsJsonPrimitive("secret").getAsString());
     */
    /* JADX WARN: Code restructure failed: missing block: B:58:0x021c, code lost:
    
        r0 = r0.getAsJsonObject("user");
        r2 = r0.getAsJsonPrimitive("username").getAsString();
     */
    /* JADX WARN: Code restructure failed: missing block: B:59:0x023d, code lost:
    
        if (r0.has("global_name") == false) goto L63;
     */
    /* JADX WARN: Code restructure failed: missing block: B:61:0x024b, code lost:
    
        if (r0.get("global_name").isJsonPrimitive() == false) goto L63;
     */
    /* JADX WARN: Code restructure failed: missing block: B:62:0x024e, code lost:
    
        r3 = r0.getAsJsonPrimitive("global_name").getAsString();
     */
    /* JADX WARN: Code restructure failed: missing block: B:64:0x0265, code lost:
    
        if (r0.has("discriminator") == false) goto L69;
     */
    /* JADX WARN: Code restructure failed: missing block: B:66:0x0273, code lost:
    
        if (r0.get("discriminator").isJsonPrimitive() == false) goto L69;
     */
    /* JADX WARN: Code restructure failed: missing block: B:67:0x0276, code lost:
    
        r4 = r0.getAsJsonPrimitive("discriminator").getAsString();
     */
    /* JADX WARN: Code restructure failed: missing block: B:68:0x0287, code lost:
    
        r5 = java.lang.Long.parseLong(r0.getAsJsonPrimitive("id").getAsString());
     */
    /* JADX WARN: Code restructure failed: missing block: B:69:0x029d, code lost:
    
        if (r0.has("avatar") == false) goto L75;
     */
    /* JADX WARN: Code restructure failed: missing block: B:71:0x02ab, code lost:
    
        if (r0.get("avatar").isJsonPrimitive() == false) goto L75;
     */
    /* JADX WARN: Code restructure failed: missing block: B:72:0x02ae, code lost:
    
        r6 = r0.getAsJsonPrimitive("avatar").getAsString();
     */
    /* JADX WARN: Code restructure failed: missing block: B:73:0x02bd, code lost:
    
        r0 = new com.jagrosh.discordipc.entities.User(r2, r3, r4, r5, r6);
        r0 = r10.listener;
     */
    /* JADX WARN: Code restructure failed: missing block: B:74:0x02cf, code lost:
    
        if (r0.has("secret") == false) goto L79;
     */
    /* JADX WARN: Code restructure failed: missing block: B:75:0x02d2, code lost:
    
        r2 = r0.getAsJsonObject("secret").getAsString();
     */
    /* JADX WARN: Code restructure failed: missing block: B:76:0x02e1, code lost:
    
        r0.onActivityJoinRequest(r11, r2, r0);
     */
    /* JADX WARN: Code restructure failed: missing block: B:77:0x02e0, code lost:
    
        r2 = null;
     */
    /* JADX WARN: Code restructure failed: missing block: B:78:0x02bc, code lost:
    
        r6 = null;
     */
    /* JADX WARN: Code restructure failed: missing block: B:79:0x0284, code lost:
    
        r4 = "0";
     */
    /* JADX WARN: Code restructure failed: missing block: B:80:0x025c, code lost:
    
        r3 = null;
     */
    /* JADX WARN: Code restructure failed: missing block: B:84:0x02ee, code lost:
    
        r16 = move-exception;
     */
    /* JADX WARN: Code restructure failed: missing block: B:85:0x02f0, code lost:
    
        getCurrentLogger(com.jagrosh.discordipc.IPCClient.LOGGER).error(java.lang.String.format("Exception when handling event: %s", r16));
     */
    /* JADX WARN: Removed duplicated region for block: B:104:0x035c  */
    /* JADX WARN: Removed duplicated region for block: B:106:? A[RETURN, SYNTHETIC] */
    /*
        Code decompiled incorrectly, please refer to instructions dump.
    */
    public void readPipe(IPCClient iPCClient) {
        while (true) {
            try {
                Packet read = this.pipe.read();
                if (read.getOp() == Packet.OpCode.CLOSE) {
                    this.pipe.setStatus(PipeStatus.DISCONNECTED);
                    if (this.listener != null) {
                        this.listener.onClose(iPCClient, read.getJson());
                    }
                    return;
                }
                JsonObject json = read.getJson();
                if (json != null) {
                    Event of = Event.of((!json.has("evt") || json.get("evt").isJsonNull()) ? null : json.getAsJsonPrimitive("evt").getAsString());
                    String asString = (!json.has("nonce") || json.get("nonce").isJsonNull()) ? null : json.getAsJsonPrimitive("nonce").getAsString();
                    switch (of) {
                        case NULL:
                            if (asString != null && this.callbacks.containsKey(asString)) {
                                this.callbacks.remove(asString).succeed(read);
                                break;
                            }
                            break;
                        case ERROR:
                            if (asString != null && this.callbacks.containsKey(asString)) {
                                this.callbacks.remove(asString).fail((json.has("data") && json.getAsJsonObject("data").has("message")) ? json.getAsJsonObject("data").getAsJsonObject("message").getAsString() : null);
                                break;
                            }
                            break;
                        case ACTIVITY_JOIN:
                            if (this.debugMode) {
                                getCurrentLogger(LOGGER).info("[DEBUG] Reading thread received a 'join' event.");
                                break;
                            }
                            break;
                        case ACTIVITY_SPECTATE:
                            if (this.debugMode) {
                                getCurrentLogger(LOGGER).info("[DEBUG] Reading thread received a 'spectate' event.");
                                break;
                            }
                            break;
                        case ACTIVITY_JOIN_REQUEST:
                            if (this.debugMode) {
                                getCurrentLogger(LOGGER).info("[DEBUG] Reading thread received a 'join request' event.");
                                break;
                            }
                            break;
                        case UNKNOWN:
                            if (this.debugMode) {
                                getCurrentLogger(LOGGER).info("[DEBUG] Reading thread encountered an event with an unknown type: " + json.getAsJsonPrimitive("evt").getAsString());
                                break;
                            }
                            break;
                    }
                }
            } catch (IOException | JsonParseException e) {
                getCurrentLogger(LOGGER).error(String.format("Reading thread encountered an Exception: %s", e));
                this.pipe.setStatus(PipeStatus.DISCONNECTED);
                if (this.listener == null) {
                    return;
                }
                this.RECONNECT_TIME_MS.reset();
                updateReconnectTime();
                this.listener.onDisconnect(iPCClient, e);
                return;
            }
        }
    }

    public void updateReconnectTime() {
        this.nextDelay = System.currentTimeMillis() + this.RECONNECT_TIME_MS.nextDelay();
    }
}
