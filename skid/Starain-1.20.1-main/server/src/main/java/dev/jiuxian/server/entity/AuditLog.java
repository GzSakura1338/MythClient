package dev.jiuxian.server.entity;

import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.table.DatabaseTable;
import lombok.Data;
import lombok.NoArgsConstructor;

@Data
@NoArgsConstructor
@DatabaseTable(tableName = "audit_logs")
public class AuditLog {
    @DatabaseField(generatedId = true)
    private int id;

    @DatabaseField(canBeNull = false)
    private String operator; // Who performed the action

    @DatabaseField(canBeNull = false)
    private String action; // Action type (e.g., LOGIN, BAN)

    @DatabaseField
    private String target; // Target user/entity (optional)

    @DatabaseField
    private String details; // Extra info

    @DatabaseField
    private String ip;

    @DatabaseField
    private long timestamp;
}
