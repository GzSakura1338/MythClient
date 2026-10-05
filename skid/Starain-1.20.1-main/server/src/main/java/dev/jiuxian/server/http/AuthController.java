//package dev.jiuxian.server.http;
//
//import com.google.gson.Gson;
//import dev.jiuxian.common.dto.ApiResponse;
//import dev.jiuxian.common.dto.ChangePasswordRequest;
//import dev.jiuxian.common.dto.LoginRequest;
//import dev.jiuxian.common.dto.RedeemRequest;
//import dev.jiuxian.common.dto.RegisterRequest;
//import dev.jiuxian.server.entity.Key;
//import dev.jiuxian.server.entity.Settings;
//import dev.jiuxian.server.entity.User;
//import dev.jiuxian.common.utils.EncryptionUtil;
//import dev.jiuxian.server.config.Config;
//import dev.jiuxian.server.database.DatabaseManager;
//import dev.jiuxian.server.manager.LogManager;
//import dev.jiuxian.server.manager.TokenManager;
//import io.javalin.http.Context;
//import org.mindrot.jbcrypt.BCrypt;
//
//import java.sql.SQLException;
//
//public class AuthController {
//    private static final Gson gson = new Gson();
//
//    public static void register(Context ctx) {
//        handleRequest(ctx, RegisterRequest.class, (req) -> {
//            // Check Blacklist
//            if (DatabaseManager.getBlacklistDao().queryBuilder().where().eq("hwid", req.getHwid()).countOf() > 0) {
//                LogManager.log(req.getUsername(), "REGISTER_FAIL", null, "Blacklisted HWID: " + req.getHwid(), ctx.ip());
//                return ApiResponse.error("HWID is blacklisted");
//            }
//
//            // Check key
//            Key key = DatabaseManager.getKeyDao().queryBuilder().where().eq("content", req.getKey()).and().eq("used", false).queryForFirst();
//            if (key == null) {
//                LogManager.log(req.getUsername(), "REGISTER_FAIL", null, "Invalid key: " + req.getKey(), ctx.ip());
//                return ApiResponse.error("Invalid or used key");
//            }
//
//            // Check username
//            if (DatabaseManager.getUserDao().queryBuilder().where().eq("username", req.getUsername()).countOf() > 0) {
//                return ApiResponse.error("Username taken");
//            }
//
//            // Check IP limit
//            String ip = ctx.ip();
//            long ipCount = DatabaseManager.getUserDao().queryBuilder().where().eq("registerIp", ip).countOf();
//            if (ipCount >= 3) {
//                LogManager.log(req.getUsername(), "REGISTER_FAIL", null, "IP limit reached: " + ip, ip);
//                return ApiResponse.error("Registration limit reached for this IP");
//            }
//
//            // Create user
//            User user = new User();
//            user.setUsername(req.getUsername());
//            user.setPassword(BCrypt.hashpw(req.getPassword(), BCrypt.gensalt()));
//            user.setHwid(req.getHwid());
//            user.setExpiryDate(System.currentTimeMillis() + key.getDuration());
//            user.setRegisterIp(ip);
//            DatabaseManager.getUserDao().create(user);
//
//            // Update key
//            key.setUsed(true);
//            key.setUsedBy(user);
//            DatabaseManager.getKeyDao().update(key);
//
//            LogManager.log(user.getUsername(), "REGISTER", user.getUsername(), "Success", ip);
//            String token = TokenManager.createToken(user.getUsername());
//            String motd = getMotd();
//            return ApiResponse.success("Registered successfully", token, motd);
//        });
//    }
//
//    public static void login(Context ctx) {
//        handleRequest(ctx, LoginRequest.class, (req) -> {
//            // Version Check
//            try {
//                Settings minVerSetting = DatabaseManager.getSettingsDao().queryForId("min_version");
//                if (minVerSetting != null) {
//                    String minVersion = minVerSetting.getValue();
//                    if (compareVersion(req.getClientVersion(), minVersion) < 0) {
//                        return ApiResponse.error("Update required. Min version: " + minVersion);
//                    }
//                }
//            } catch (SQLException e) {
//                // Ignore version check if db error or setting missing
//            }
//
//            User user = DatabaseManager.getUserDao().queryBuilder().where().eq("username", req.getUsername()).queryForFirst();
//            if (user == null || !BCrypt.checkpw(req.getPassword(), user.getPassword())) {
//                LogManager.log(req.getUsername(), "LOGIN_FAIL", null, "Invalid credentials", ctx.ip());
//                return ApiResponse.error("Invalid credentials");
//            }
//
//            if (user.isBanned()) return ApiResponse.error("User banned");
//
//            if (user.getBanExpiryDate() > 0 && user.getBanExpiryDate() > System.currentTimeMillis()) {
//                long remaining = (user.getBanExpiryDate() - System.currentTimeMillis()) / 1000;
//                return ApiResponse.error("User temporarily banned. Expires in " + remaining + "s");
//            } else if (user.getBanExpiryDate() > 0) {
//                 // Auto unban
//                 user.setBanExpiryDate(0);
//                 DatabaseManager.getUserDao().update(user);
//            }
//
//            // Check Blacklist
//            if (DatabaseManager.getBlacklistDao().queryBuilder().where().eq("hwid", req.getHwid()).countOf() > 0) {
//                 LogManager.log(user.getUsername(), "LOGIN_FAIL", null, "Blacklisted HWID", ctx.ip());
//                 return ApiResponse.error("HWID is blacklisted");
//            }
//
//            if (user.getExpiryDate() < System.currentTimeMillis()) {
//                return ApiResponse.error("Subscription expired");
//            }
//
//            if (!req.getHwid().equals(user.getHwid())) {
//                 // Optional: Allow HWID reset or block
//                 LogManager.log(user.getUsername(), "LOGIN_FAIL", null, "HWID Mismatch", ctx.ip());
//                 return ApiResponse.error("Invalid HWID");
//            }
//
//            LogManager.log(user.getUsername(), "LOGIN", null, "Success", ctx.ip());
//            String token = TokenManager.createToken(user.getUsername());
//            String motd = getMotd();
//            return ApiResponse.success("Login successful", token, motd);
//        });
//    }
//
//    public static void redeem(Context ctx) {
//         handleRequest(ctx, RedeemRequest.class, (req) -> {
//            User user = DatabaseManager.getUserDao().queryBuilder().where().eq("username", req.getUsername()).queryForFirst();
//            if (user == null) return ApiResponse.error("User not found");
//
//            Key key = DatabaseManager.getKeyDao().queryBuilder().where().eq("content", req.getKey()).and().eq("used", false).queryForFirst();
//            if (key == null) return ApiResponse.error("Invalid key");
//
//            user.setExpiryDate(Math.max(System.currentTimeMillis(), user.getExpiryDate()) + key.getDuration());
//            DatabaseManager.getUserDao().update(user);
//
//            key.setUsed(true);
//            key.setUsedBy(user);
//            DatabaseManager.getKeyDao().update(key);
//
//            LogManager.log(user.getUsername(), "REDEEM", null, "Key: " + req.getKey(), ctx.ip());
//            return ApiResponse.success("Redeemed successfully", null, null);
//         });
//    }
//
//    public static void changePassword(Context ctx) {
//        handleRequest(ctx, ChangePasswordRequest.class, (req) -> {
//            User user = DatabaseManager.getUserDao().queryBuilder().where().eq("username", req.getUsername()).queryForFirst();
//            if (user == null || !BCrypt.checkpw(req.getOldPassword(), user.getPassword())) {
//                return ApiResponse.error("Invalid credentials");
//            }
//
//            user.setPassword(BCrypt.hashpw(req.getNewPassword(), BCrypt.gensalt()));
//            DatabaseManager.getUserDao().update(user);
//
//            LogManager.log(user.getUsername(), "CHANGE_PASS", null, "Success", ctx.ip());
//            return ApiResponse.success("Password changed", null, null);
//        });
//    }
//
//    private static int compareVersion(String v1, String v2) {
//        if (v1 == null) return -1;
//        if (v2 == null) return 1;
//        String[] p1 = v1.split("\\.");
//        String[] p2 = v2.split("\\.");
//        int len = Math.max(p1.length, p2.length);
//        for (int i = 0; i < len; i++) {
//            int n1 = i < p1.length ? Integer.parseInt(p1[i]) : 0;
//            int n2 = i < p2.length ? Integer.parseInt(p2[i]) : 0;
//            if (n1 != n2) return n1 - n2;
//        }
//        return 0;
//    }
//
//    private static String getMotd() {
//        try {
//            Settings s = DatabaseManager.getSettingsDao().queryForId("motd");
//            return s != null ? s.getValue() : "";
//        } catch (SQLException e) {
//            return "";
//        }
//    }
//
//    private static <T> void handleRequest(Context ctx, Class<T> clazz, RequestHandler<T> handler) {
//        try {
//            String encryptedBody = ctx.body();
//            String json = EncryptionUtil.decrypt(encryptedBody, Config.get().getAesSecret());
//            T req = gson.fromJson(json, clazz);
//
//            // Validate timestamp (Assuming all request DTOs have getTimestamp via reflection or common interface,
//            // but for simplicity I'll cast or just trust the structure since I defined them)
//            // Ideally, make DTOs implement an interface `Timestamped`.
//            // For now, I'll use reflection or just assume valid JSON.
//            // Let's check timestamp manually via reflection or assume the generic handler knows.
//            // To be safe, I'll just access the field if possible or rely on the specific method logic.
//            // Wait, I can't easily access getTimestamp generically without an interface.
//            // I'll add an interface or just do it inside the lambda.
//            // Actually, EncryptionUtil.validateTimestamp is required.
//
//            long timestamp = 0;
//            if (req instanceof RegisterRequest) timestamp = ((RegisterRequest) req).getTimestamp();
//            else if (req instanceof LoginRequest) timestamp = ((LoginRequest) req).getTimestamp();
//            else if (req instanceof RedeemRequest) timestamp = ((RedeemRequest) req).getTimestamp();
//
//            if (!EncryptionUtil.validateTimestamp(timestamp)) {
//                sendResponse(ctx, ApiResponse.error("Request expired"));
//                return;
//            }
//
//            ApiResponse response = handler.handle(req);
//            sendResponse(ctx, response);
//        } catch (Exception e) {
//            e.printStackTrace();
//            try {
//                sendResponse(ctx, ApiResponse.error("Server error: " + e.getMessage()));
//            } catch (Exception ex) {
//                ctx.status(500);
//            }
//        }
//    }
//
//    private static void sendResponse(Context ctx, ApiResponse response) throws Exception {
//        String json = gson.toJson(response);
//        String encrypted = EncryptionUtil.encrypt(json, Config.get().getAesSecret());
//        ctx.result(encrypted);
//    }
//
//    @FunctionalInterface
//    interface RequestHandler<T> {
//        ApiResponse handle(T req) throws Exception;
//    }
//}
