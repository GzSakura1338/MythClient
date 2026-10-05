package com.jagrosh.discordipc.entities.pipe;

import com.jagrosh.discordipc.IPCClient;
import com.jagrosh.discordipc.entities.Callback;
import java.io.File;
import java.io.FileWriter;
import java.util.HashMap;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:com/jagrosh/discordipc/entities/pipe/MacPipe.class */
public class MacPipe extends UnixPipe {
    public MacPipe(IPCClient iPCClient, HashMap<String, Callback> hashMap, File file) throws java.io.IOException {
        super(iPCClient, hashMap, file);
    }

    public void registerCommand(String str, String str2) {
        String str3 = System.getenv("HOME");
        if (str3 == null) {
            throw new RuntimeException("Unable to find user HOME directory");
        }
        String str4 = str3 + "/Library/Application Support/discord";
        if (!mkdir(str4)) {
            throw new RuntimeException("Failed to create directory '" + str4 + "'");
        }
        String str5 = str4 + "/games";
        if (!mkdir(str5)) {
            throw new RuntimeException("Failed to create directory '" + str5 + "'");
        }
        String str6 = str5 + "/" + str + ".json";
        try {
            FileWriter fileWriter = new FileWriter(str6);
            try {
                fileWriter.write("{\"command\": \"" + str2 + "\"}");
                fileWriter.close();
            } finally {
            }
        } catch (Exception e) {
            throw new RuntimeException("Failed to write fame info into '" + str6 + "'");
        }
    }

    public void registerUrl(String str) {
        throw new UnsupportedOperationException("MacOS URL registration is not supported at this time.");
    }

    @Override // com.jagrosh.discordipc.entities.pipe.UnixPipe, com.jagrosh.discordipc.entities.pipe.Pipe
    public void registerApp(String str, String str2) {
        try {
            if (str2 != null) {
                registerCommand(str, str2);
            } else {
                registerUrl(str);
            }
        } catch (Exception e) {
            throw new RuntimeException("Failed to register " + (str2 == null ? "url" : "command"), e);
        }
    }

    @Override // com.jagrosh.discordipc.entities.pipe.UnixPipe, com.jagrosh.discordipc.entities.pipe.Pipe
    public void registerSteamGame(String str, String str2) {
        registerApp(str, "steam://rungameid/" + str2);
    }
}
