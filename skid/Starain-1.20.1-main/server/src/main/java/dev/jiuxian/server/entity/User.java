package dev.jiuxian.server.entity;

import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.table.DatabaseTable;
import lombok.Data;
import lombok.NoArgsConstructor;

@Data
@NoArgsConstructor
@DatabaseTable(tableName = "users")
public class User {
    @DatabaseField(generatedId = true)
    private int id;
    
    @DatabaseField(unique = true, canBeNull = false)
    private String username;
    
    @DatabaseField(canBeNull = false)
    private String password;
    
    @DatabaseField
    private String hwid;
    
    @DatabaseField(defaultValue = "false")
    private boolean banned;
    
    @DatabaseField
    private long expiryDate; // Timestamp

    @DatabaseField
    private long banExpiryDate; // Timestamp, 0 = no temp ban

    @DatabaseField
    private String qq;
}
