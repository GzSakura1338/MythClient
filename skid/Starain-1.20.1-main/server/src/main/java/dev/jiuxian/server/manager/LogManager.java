package dev.jiuxian.server.manager;

import dev.jiuxian.server.entity.AuditLog;
import dev.jiuxian.server.database.DatabaseManager;
import java.sql.SQLException;

public class LogManager {
    public static void log(String operator, String action, String target, String details, String ip) {
        try {
            AuditLog log = new AuditLog();
            log.setOperator(operator);
            log.setAction(action);
            log.setTarget(target);
            log.setDetails(details);
            log.setIp(ip);
            log.setTimestamp(System.currentTimeMillis());
            DatabaseManager.getAuditLogDao().create(log);
        } catch (SQLException e) {
            e.printStackTrace();
        }
    }
}
