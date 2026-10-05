package dev.jiuxian.server.http;

import com.google.gson.Gson;
import dev.jiuxian.server.config.Config;
import io.javalin.Javalin;
import io.javalin.json.JsonMapper;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.UncheckedIOException;
import java.lang.reflect.Type;
import java.nio.charset.StandardCharsets;
import java.util.stream.Stream;

public class HttpServer {
    private Javalin app;

    private static final Gson gson = new Gson();
    private static final JsonMapper jsonMapper = new JsonMapper() {
        @Override
        public String toJsonString(Object obj, Type type) {
            return gson.toJson(obj, type);
        }

        @Override
        public <T> T fromJsonString(String json, Type targetType) {
            return gson.fromJson(json, targetType);
        }

        @Override
        public InputStream toJsonStream(Object obj, Type type) {
            byte[] bytes = gson.toJson(obj, type).getBytes(StandardCharsets.UTF_8);
            return new ByteArrayInputStream(bytes);
        }

        @Override
        public <T> T fromJsonStream(InputStream jsonStream, Type targetType) {
            try {
                String json = new String(jsonStream.readAllBytes(), StandardCharsets.UTF_8);
                return gson.fromJson(json, targetType);
            } catch (IOException e) {
                throw new UncheckedIOException(e);
            }
        }

        @Override
        public void writeToOutputStream(Stream<?> stream, OutputStream outputStream) {
            try (OutputStreamWriter writer = new OutputStreamWriter(outputStream, StandardCharsets.UTF_8)) {
                gson.toJson(stream.toList(), writer);
            } catch (IOException e) {
                throw new UncheckedIOException(e);
            }
        }
    };

    public void start() {
        app = Javalin.create(cfg -> cfg.jsonMapper(jsonMapper)).start(Config.get().getHttpPort());

        app.get("/admin/login", ctx -> {
            if (ctx.sessionAttribute("admin") != null) {
                ctx.redirect("/admin");
            } else {
                ctx.contentType("text/html").result(getClass().getResourceAsStream("/login.html"));
            }
        });

        app.get("/admin", ctx -> {
            if (ctx.sessionAttribute("admin") == null) {
                ctx.redirect("/admin/login");
            } else {
                ctx.contentType("text/html").result(getClass().getResourceAsStream("/admin.html"));
            }
        });

        app.get("/admin/css/login.css", ctx -> {
            ctx.contentType("text/css").result(getClass().getResourceAsStream("/css/login.css"));
        });

        app.get("/admin/js/login.js", ctx -> {
            ctx.contentType("text/javascript").result(getClass().getResourceAsStream("/js/login.js"));
        });

        app.get("/admin/css/admin.css", ctx -> {
            ctx.contentType("text/css").result(getClass().getResourceAsStream("/css/admin.css"));
        });

        app.get("/admin/js/admin.js", ctx -> {
            ctx.contentType("text/javascript").result(getClass().getResourceAsStream("/js/admin.js"));
        });

        app.post("/admin/login", ctx -> {
            if (Config.get().getAdminUsername().equals(ctx.formParam("username")) &&
                Config.get().getAdminPassword().equals(ctx.formParam("password"))) {
                ctx.sessionAttribute("admin", true);
                ctx.redirect("/admin");
            } else {
                ctx.redirect("/admin/login?error=1");
            }
        });

        app.get("/admin/logout", ctx -> {
            ctx.sessionAttribute("admin", null);
            ctx.redirect("/admin/login");
        });

        app.before("/admin/*", ctx -> {
            if (ctx.path().equals("/admin/login") ||
                ctx.path().equals("/admin/css/login.css") ||
                ctx.path().equals("/admin/js/login.js")) return;

            if (ctx.sessionAttribute("admin") == null) {
                if (ctx.path().startsWith("/admin/api")) {
                    ctx.status(401).result("Unauthorized");
                } else {
                    ctx.redirect("/admin/login");
                }
            }
        });

        app.get("/admin/api/stats", AdminController::getStats);
        app.get("/admin/api/users", AdminController::getUsers);
        app.post("/admin/api/users/{id}/ban", AdminController::toggleBan);
        app.post("/admin/api/users/{id}/reset-password", AdminController::resetPassword);
        app.post("/admin/api/users/{id}/kick", AdminController::kickUser);
        app.post("/admin/api/users/{id}/reset-hwid", AdminController::resetHwid);
        app.get("/admin/api/keys", AdminController::getKeys);
        app.post("/admin/api/keys/generate", AdminController::generateKeys);
        app.post("/admin/api/keys/delete", AdminController::deleteKeys);

        app.get("/admin/api/online", AdminController::getOnlineUsers);
        app.post("/admin/api/broadcast", AdminController::broadcast);

        app.get("/admin/api/blacklist", AdminController::getBlacklist);
        app.post("/admin/api/blacklist", AdminController::addBlacklist);
        app.delete("/admin/api/blacklist/{id}", AdminController::removeBlacklist);

        app.get("/admin/api/settings", AdminController::getSettings);
        app.post("/admin/api/settings", AdminController::updateSettings);

        app.get("/admin/api/logs", AdminController::getAuditLogs);
    }

    public void stop() {
        if (app != null) app.stop();
    }
}
