package dev.jiuxian.server.entity;

import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.table.DatabaseTable;
import lombok.Data;
import lombok.NoArgsConstructor;

@Data
@NoArgsConstructor
@DatabaseTable(tableName = "settings")
public class Settings {
    @DatabaseField(id = true)
    private String key;
    
    @DatabaseField
    private String value;
}
