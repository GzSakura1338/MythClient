package dev.jiuxian.server.entity;

import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.table.DatabaseTable;
import lombok.Data;
import lombok.NoArgsConstructor;

@Data
@NoArgsConstructor
@DatabaseTable(tableName = "keys")
public class Key {
    @DatabaseField(generatedId = true)
    private int id;
    
    @DatabaseField(unique = true, canBeNull = false)
    private String content; // The key itself
    
    @DatabaseField(canBeNull = false)
    private long duration; // In milliseconds
    
    @DatabaseField(defaultValue = "false")
    private boolean used;
    
    @DatabaseField(foreign = true, foreignAutoRefresh = true)
    private User usedBy;

    @DatabaseField
    private String remark;
}
