package dev.jiuxian.common.packet;

import lombok.Data;

import javax.net.ssl.HttpsURLConnection;
import java.net.URL;
import java.text.SimpleDateFormat;
import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;
import java.util.Locale;

import static dev.jiuxian.common.utils.EncryptionUtil.getOnlineTime;

@Data
public abstract class Packet {
    private String type;
    private long time;
    
    public Packet(String type) {
        this.type = type;
        time = System.currentTimeMillis();
    }
}
