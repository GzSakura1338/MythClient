package dev.jiuxian.server;

import dev.jiuxian.server.config.Config;
import dev.jiuxian.server.database.DatabaseManager;
import dev.jiuxian.server.http.HttpServer;
import dev.jiuxian.server.socket.SocketServer;
import lombok.extern.slf4j.Slf4j;

@Slf4j
public class ServerMain {
    public static void main(String[] args) {
        try {
            log.info("Loading Config...");
            Config.load();
            
            log.info("Initializing Database...");
            DatabaseManager.init();
            
            log.info("Starting Socket Server...");
            new SocketServer().start();
            
            log.info("Starting HTTP Server...");
            new HttpServer().start();
            
            log.info("Verify Server started!");
        } catch (Exception e) {
            log.error("Failed to start server", e);
            System.exit(1);
        }
    }
}
