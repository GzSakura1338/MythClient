package dev.jiuxian.server.database;

import com.j256.ormlite.dao.Dao;
import com.j256.ormlite.dao.DaoManager;
import com.j256.ormlite.jdbc.JdbcConnectionSource;
import com.j256.ormlite.support.ConnectionSource;
import com.j256.ormlite.table.TableUtils;
import dev.jiuxian.server.entity.AuditLog;
import dev.jiuxian.server.entity.Blacklist;
import dev.jiuxian.server.entity.ChatMessage;
import dev.jiuxian.server.entity.Key;
import dev.jiuxian.server.entity.Settings;
import dev.jiuxian.server.entity.User;
import lombok.Getter;

public class DatabaseManager {
    @Getter private static Dao<User, Integer> userDao;
    @Getter private static Dao<Key, Integer> keyDao;
    @Getter private static Dao<Settings, String> settingsDao;
    @Getter private static Dao<Blacklist, Integer> blacklistDao;
    @Getter private static Dao<AuditLog, Integer> auditLogDao;
    @Getter private static Dao<ChatMessage, Integer> chatMessageDao;
    private static ConnectionSource connectionSource;

    public static void init() throws Exception {
        String databaseUrl = "jdbc:sqlite:database.db";
        connectionSource = new JdbcConnectionSource(databaseUrl);

        userDao = DaoManager.createDao(connectionSource, User.class);
        keyDao = DaoManager.createDao(connectionSource, Key.class);
        settingsDao = DaoManager.createDao(connectionSource, Settings.class);
        blacklistDao = DaoManager.createDao(connectionSource, Blacklist.class);
        auditLogDao = DaoManager.createDao(connectionSource, AuditLog.class);
        chatMessageDao = DaoManager.createDao(connectionSource, ChatMessage.class);

        TableUtils.createTableIfNotExists(connectionSource, User.class);
        TableUtils.createTableIfNotExists(connectionSource, Key.class);
        TableUtils.createTableIfNotExists(connectionSource, Settings.class);
        TableUtils.createTableIfNotExists(connectionSource, Blacklist.class);
        TableUtils.createTableIfNotExists(connectionSource, AuditLog.class);
        TableUtils.createTableIfNotExists(connectionSource, ChatMessage.class);
        
        try {
            keyDao.executeRaw("ALTER TABLE `keys` ADD COLUMN `remark` VARCHAR;");
        } catch (Exception e) {
            // Column likely exists or table just created
        }

        try {
            userDao.executeRaw("ALTER TABLE `users` ADD COLUMN `banExpiryDate` BIGINT DEFAULT 0;");
        } catch (Exception e) {
        }
        
        try {
            userDao.executeRaw("ALTER TABLE `users` ADD COLUMN `registerIp` VARCHAR;");
        } catch (Exception e) {
        }
        
        initSettings();
    }
    
    public static void close() throws Exception {
        if (connectionSource != null) {
            connectionSource.close();
        }
    }
    
    private static void initSettings() throws Exception {
        createSettingIfNotExists("motd", "Welcome to Verify System");
        createSettingIfNotExists("latest_version", "1.0.0");
        createSettingIfNotExists("min_version", "1.0.0");
    }

    private static void createSettingIfNotExists(String key, String value) throws Exception {
        if (!settingsDao.idExists(key)) {
            Settings s = new Settings();
            s.setKey(key);
            s.setValue(value);
            settingsDao.create(s);
        }
    }
}
