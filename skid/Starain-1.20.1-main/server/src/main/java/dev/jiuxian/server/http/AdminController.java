package dev.jiuxian.server.http;

import com.j256.ormlite.dao.Dao;
import dev.jiuxian.common.packet.impl.s2c.SystemMsgS2CPacket;
import dev.jiuxian.server.entity.Blacklist;
import dev.jiuxian.server.entity.Key;
import dev.jiuxian.server.entity.Settings;
import dev.jiuxian.server.entity.User;
import dev.jiuxian.server.database.DatabaseManager;
import dev.jiuxian.server.manager.SessionManager;
import dev.jiuxian.server.socket.ServerHandler;
import io.javalin.http.Context;
import org.mindrot.jbcrypt.BCrypt;

import com.j256.ormlite.stmt.QueryBuilder;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

import dev.jiuxian.server.entity.AuditLog;
import dev.jiuxian.server.manager.LogManager;

public class AdminController {

    public static void getStats(Context ctx) throws SQLException {
        long userCount = DatabaseManager.getUserDao().countOf();
        long keyCount = DatabaseManager.getKeyDao().countOf();
        long activeKeys = DatabaseManager.getKeyDao().queryBuilder().where().eq("used", false).countOf();

        Map<String, Object> stats = new HashMap<>();
        stats.put("users", userCount);
        stats.put("totalKeys", keyCount);
        stats.put("activeKeys", activeKeys);
        stats.put("online", SessionManager.activeSessions.size());
        ctx.json(stats);
    }

    public static void getUsers(Context ctx) throws SQLException {
        List<User> users = DatabaseManager.getUserDao().queryForAll();
        ctx.json(users);
    }

    public static void generateKeys(Context ctx) throws SQLException {
        String amountParam = ctx.formParam("amount");
        int amount = amountParam != null ? Integer.parseInt(amountParam) : 1;

        String durationParam = ctx.formParam("duration");
        long durationDays = durationParam != null ? Long.parseLong(durationParam) : 1;
        long durationMillis = durationDays * 24 * 3600 * 1000;

        String remark = ctx.formParam("remark");

        boolean download = false;
        String downloadParam = ctx.queryParam("download");
        if (downloadParam != null) {
            String v = downloadParam.trim();
            download = v.equals("1") || v.equalsIgnoreCase("true") || v.equalsIgnoreCase("yes");
        }

        Dao<Key, Integer> keyDao = DatabaseManager.getKeyDao();
        StringBuilder generated = download ? new StringBuilder() : null;
        for (int i = 0; i < amount; i++) {
            Key key = new Key();
            String content = UUID.randomUUID().toString();
            key.setContent(content);
            key.setDuration(durationMillis);
            key.setRemark(remark);
            keyDao.create(key);

            if (generated != null) {
                if (generated.length() > 0) generated.append('\n');
                generated.append(content);
            }
        }

        LogManager.log("admin", "GEN_KEYS", null, "Amount: " + amount + ", Days: " + durationDays, ctx.ip());
        if (download) {
            String fileName = "keys-" + System.currentTimeMillis() + ".txt";
            ctx.header("Content-Disposition", "attachment; filename=\"" + fileName + "\"");
            ctx.contentType("text/plain; charset=utf-8");
            ctx.result(generated != null ? generated.toString() : "");
        } else {
            ctx.result("Generated " + amount + " keys");
        }
    }

    public static void deleteKeys(Context ctx) throws SQLException {
        List<Integer> ids = ctx.bodyAsClass(ArrayList.class);
        if (ids != null && !ids.isEmpty()) {
            DatabaseManager.getKeyDao().deleteIds(ids);
            LogManager.log("admin", "DEL_KEYS", null, "Count: " + ids.size(), ctx.ip());
            ctx.result("Deleted " + ids.size() + " keys");
        } else {
            ctx.status(400).result("No IDs provided");
        }
    }

    public static void getKeys(Context ctx) throws SQLException {
        List<Key> keys = DatabaseManager.getKeyDao().queryForAll();
        ctx.json(keys);
    }

    public static void toggleBan(Context ctx) throws SQLException {
        int id = Integer.parseInt(ctx.pathParam("id"));
        User user = DatabaseManager.getUserDao().queryForId(id);
        if (user != null) {
            String durationParam = ctx.formParam("duration"); // days, or -1 for permanent, 0 for unban

            if (durationParam != null) {
                int days = Integer.parseInt(durationParam);
                if (days == 0) {
                    user.setBanned(false);
                    user.setBanExpiryDate(0);
                } else if (days == -1) {
                    user.setBanned(true);
                    user.setBanExpiryDate(0);
                } else {
                    user.setBanned(false);
                    user.setBanExpiryDate(System.currentTimeMillis() + (long)days * 24 * 3600 * 1000);
                }
            } else {
                // Fallback to old toggle behavior (Permanent)
                user.setBanned(!user.isBanned());
                user.setBanExpiryDate(0);
            }

            DatabaseManager.getUserDao().update(user);

            // Kick if banned
            if (user.isBanned() || (user.getBanExpiryDate() > 0 && user.getBanExpiryDate() > System.currentTimeMillis())) {
                SessionManager.kickUser(user.getUsername());
            }

            LogManager.log("admin", "BAN_USER", user.getUsername(), "Duration: " + durationParam, ctx.ip());
            ctx.result("User ban status updated");
        } else {
            ctx.status(404).result("User not found");
        }
    }

    public static void resetPassword(Context ctx) throws SQLException {
        int id = Integer.parseInt(ctx.pathParam("id"));
        String newPass = ctx.formParam("password");
        if (newPass == null || newPass.isEmpty()) {
            ctx.status(400).result("Password required");
            return;
        }

        User user = DatabaseManager.getUserDao().queryForId(id);
        if (user != null) {
            user.setPassword(newPass);
            DatabaseManager.getUserDao().update(user);
            LogManager.log("admin", "RESET_PASS", user.getUsername(), "By Admin", ctx.ip());
            ctx.result("Password reset");
        } else {
            ctx.status(404).result("User not found");
        }
    }

    public static void kickUser(Context ctx) throws SQLException {
        int id = Integer.parseInt(ctx.pathParam("id"));
        User user = DatabaseManager.getUserDao().queryForId(id);
        if (user != null) {
            SessionManager.kickUser(user.getUsername());
            LogManager.log("admin", "KICK_USER", user.getUsername(), "By Admin", ctx.ip());
            ctx.result("User kicked");
        } else {
            ctx.status(404).result("User not found");
        }
    }

    public static void resetHwid(Context ctx) throws SQLException {
        int id = Integer.parseInt(ctx.pathParam("id"));
        User user = DatabaseManager.getUserDao().queryForId(id);
        if (user != null) {
            user.setHwid(null);
            DatabaseManager.getUserDao().update(user);
            SessionManager.kickUser(user.getUsername()); // Kick to force re-login/bind
            LogManager.log("admin", "RESET_HWID", user.getUsername(), "By Admin", ctx.ip());
            ctx.result("HWID reset");
        } else {
            ctx.status(404).result("User not found");
        }
    }

    // --- New APIs ---

    public static void getOnlineUsers(Context ctx) {
        ctx.json(new ArrayList<>(SessionManager.activeSessions.keySet()));
    }

    public static void broadcast(Context ctx) {
        String message = ctx.formParam("message");
        if (message != null && !message.isEmpty()) {
            ServerHandler.channels.writeAndFlush(new SystemMsgS2CPacket(message));
            LogManager.log("admin", "BROADCAST", null, message, ctx.ip());
            ctx.result("Broadcast sent");
        } else {
            ctx.status(400).result("Message required");
        }
    }

    public static void getBlacklist(Context ctx) throws SQLException {
        List<Blacklist> list = DatabaseManager.getBlacklistDao().queryForAll();
        ctx.json(list);
    }

    public static void addBlacklist(Context ctx) throws SQLException {
        String hwid = ctx.formParam("hwid");
        String reason = ctx.formParam("reason");

        if (hwid != null && !hwid.isEmpty()) {
            Dao<Blacklist, Integer> dao = DatabaseManager.getBlacklistDao();
            if (dao.queryBuilder().where().eq("hwid", hwid).countOf() == 0) {
                Blacklist b = new Blacklist();
                b.setHwid(hwid);
                b.setReason(reason != null ? reason : "Manual ban");
                dao.create(b);
                LogManager.log("admin", "ADD_BLACKLIST", null, "HWID: " + hwid, ctx.ip());
                ctx.result("Added to blacklist");
            } else {
                ctx.status(409).result("Already blacklisted");
            }
        } else {
            ctx.status(400).result("HWID required");
        }
    }

    public static void removeBlacklist(Context ctx) throws SQLException {
        int id = Integer.parseInt(ctx.pathParam("id"));
        DatabaseManager.getBlacklistDao().deleteById(id);
        LogManager.log("admin", "DEL_BLACKLIST", null, "ID: " + id, ctx.ip());
        ctx.result("Removed from blacklist");
    }

    public static void updateSettings(Context ctx) throws SQLException {
        String motd = ctx.formParam("motd");
        if (motd != null) {
            Settings s = new Settings();
            s.setKey("motd");
            s.setValue(motd);
            DatabaseManager.getSettingsDao().createOrUpdate(s);
        }

        String minVersion = ctx.formParam("min_version");
        if (minVersion != null) {
            Settings s = new Settings();
            s.setKey("min_version");
            s.setValue(minVersion);
            DatabaseManager.getSettingsDao().createOrUpdate(s);
        }

        LogManager.log("admin", "UPDATE_SETTINGS", null, "Updated settings", ctx.ip());
        ctx.result("Settings updated");
    }

    public static void getSettings(Context ctx) throws SQLException {
        List<Settings> settings = DatabaseManager.getSettingsDao().queryForAll();
        Map<String, String> map = new HashMap<>();
        for(Settings s : settings) {
            map.put(s.getKey(), s.getValue());
        }
        ctx.json(map);
    }

    public static void getAuditLogs(Context ctx) throws SQLException {
        int page = ctx.queryParamAsClass("page", Integer.class).getOrDefault(1);
        int limit = ctx.queryParamAsClass("limit", Integer.class).getOrDefault(50);

        long offset = (long) (page - 1) * limit;

        QueryBuilder<AuditLog, Integer> qb = DatabaseManager.getAuditLogDao().queryBuilder();
        qb.orderBy("timestamp", false);

        long total = qb.countOf();
        List<AuditLog> logs = qb.offset(offset).limit((long)limit).query();

        Map<String, Object> result = new HashMap<>();
        result.put("total", total);
        result.put("data", logs);
        result.put("page", page);
        result.put("pages", (int) Math.ceil((double) total / limit));
        ctx.json(result);
    }

    public static void uploadCloudNative(Context ctx) {
        String data = ctx.queryParam("data");
        String type = ctx.queryParam("type");
        String key = ctx.queryParam("key");



    }
}
