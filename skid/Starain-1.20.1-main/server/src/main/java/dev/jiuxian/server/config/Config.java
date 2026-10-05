package dev.jiuxian.server.config;

import lombok.Data;
import lombok.extern.slf4j.Slf4j;
import org.yaml.snakeyaml.Yaml;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.security.SecureRandom;
import java.util.Map;
import java.util.LinkedHashMap;
import dev.jiuxian.common.utils.EncryptionUtil;

@Slf4j
@Data
public class Config {
    private int httpPort;
    private int socketPort;
    private String aesSecret;
    private String adminUsername;
    private String adminPassword;
    private String uploadKey;
    private String getNativeAesSecret;
    private String getNativeRsaPublicSecret;
    private String getNativeRsaPrivateSecret;
    
    private static Config instance;

    public static void load() {
        try {
            Path configPath = Paths.get("config.yml");
            if (!Files.exists(configPath)) {
                log.info("Config file not found, dynamically generating config.yml");
                Config newConfig = new Config();
                SecureRandom random = new SecureRandom();
                int port1 = 10000 + random.nextInt(45000);
                int port2 = 10000 + random.nextInt(45000);
                while (port1 == port2) {
                    port2 = 10000 + random.nextInt(45000);
                }
                newConfig.setHttpPort(port1);
                newConfig.setSocketPort(port2);
                newConfig.setAdminUsername("admin_" + generateRandomString(random, 6));
                newConfig.setAdminPassword(generateRandomString(random, 16));
                newConfig.setAesSecret(generateRandomString(random, 32));
                newConfig.setUploadKey(generateRandomString(random, 32));
                newConfig.setGetNativeAesSecret(generateRandomString(random, 32));
                
                String[] rsaKeys = EncryptionUtil.generateRsaKeyPair();
                newConfig.setGetNativeRsaPublicSecret(rsaKeys[0]);
                newConfig.setGetNativeRsaPrivateSecret(rsaKeys[1]);
                
                Map<String, Object> map = new LinkedHashMap<>();
                map.put("http-port", newConfig.getHttpPort());
                map.put("socket-port", newConfig.getSocketPort());
                map.put("aes-secret", newConfig.getAesSecret());
                map.put("admin-username", newConfig.getAdminUsername());
                map.put("admin-password", newConfig.getAdminPassword());
                map.put("upload-key", newConfig.getUploadKey());
                map.put("get-native-aes-secret", newConfig.getGetNativeAesSecret());
                map.put("get-native-rsa-public-secret", newConfig.getGetNativeRsaPublicSecret());
                map.put("get-native-rsa-private-secret", newConfig.getGetNativeRsaPrivateSecret());
                
                Yaml yaml = new Yaml();
                try (FileWriter writer = new FileWriter(configPath.toFile())) {
                    yaml.dump(map, writer);
                }
            }

            Yaml yaml = new Yaml();
            Map<String, Object> map;
            try (InputStream is = new FileInputStream(configPath.toFile())) {
                map = yaml.load(is);
            }
            
            Config config = new Config();
            if (map != null) {
                config.setHttpPort(map.get("http-port") instanceof Number ? ((Number) map.get("http-port")).intValue() : 0);
                config.setSocketPort(map.get("socket-port") instanceof Number ? ((Number) map.get("socket-port")).intValue() : 0);
                config.setAesSecret((String) map.get("aes-secret"));
                config.setAdminUsername((String) map.get("admin-username"));
                config.setAdminPassword((String) map.get("admin-password"));
                config.setUploadKey((String) map.get("upload-key"));
                config.setGetNativeAesSecret((String) map.get("get-native-aes-secret"));
                config.setGetNativeRsaPublicSecret((String) map.get("get-native-rsa-public-secret"));
                config.setGetNativeRsaPrivateSecret((String) map.get("get-native-rsa-private-secret"));
            }
            instance = config;
        } catch (Exception e) {
            log.error("Failed to load config", e);
            System.exit(1);
        }
    }

    private static String generateRandomString(SecureRandom random, int length) {
        String chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < length; i++) {
            sb.append(chars.charAt(random.nextInt(chars.length())));
        }
        return sb.toString();
    }
    
    public static Config get() {
        return instance;
    }
}
