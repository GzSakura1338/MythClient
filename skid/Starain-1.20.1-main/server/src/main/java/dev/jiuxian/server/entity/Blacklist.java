package dev.jiuxian.server.entity;

import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.table.DatabaseTable;
import lombok.Data;
import lombok.NoArgsConstructor;

@Data
@NoArgsConstructor
@DatabaseTable(tableName = "blacklist")
public class Blacklist {
    @DatabaseField(generatedId = true)
    private int id;
    
    @DatabaseField(unique = true, canBeNull = false)
    private String hwid;
    
    @DatabaseField
    private String reason;
}
