package dev.jiuxian.server.entity;

import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.table.DatabaseTable;
import lombok.Data;
import lombok.NoArgsConstructor;

@Data
@NoArgsConstructor
@DatabaseTable(tableName = "chat_messages")
public class ChatMessage {
    @DatabaseField(generatedId = true)
    private int id;

    @DatabaseField(canBeNull = false)
    private String sender;

    @DatabaseField
    private String target; // null for broadcast

    @DatabaseField(canBeNull = false)
    private String message;

    @DatabaseField(canBeNull = false)
    private long timestamp;

    @DatabaseField(defaultValue = "false")
    private boolean read; // Only relevant for offline PMs
}
