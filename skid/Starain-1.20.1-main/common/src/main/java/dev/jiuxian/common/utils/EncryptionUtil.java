package dev.jiuxian.common.utils;

import javax.crypto.Cipher;
import javax.crypto.spec.SecretKeySpec;
import javax.net.ssl.HttpsURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;
import java.util.Base64;
import java.security.MessageDigest;
import java.util.Arrays;
import java.util.Locale;
import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.spec.X509EncodedKeySpec;
import java.security.Signature;

public class EncryptionUtil {

    private static final String ALGORITHM = "AES";
    private static final String TRANSFORMATION = "AES/ECB/PKCS5Padding"; // Using ECB for simplicity as per common loader practices, though CBC is better

    public static String encrypt(String data, String secret) throws Exception {
        SecretKeySpec key = generateKey(secret);
        Cipher cipher = Cipher.getInstance(TRANSFORMATION);
        cipher.init(Cipher.ENCRYPT_MODE, key);
        byte[] encVal = cipher.doFinal(data.getBytes(StandardCharsets.UTF_8));
        return Base64.getEncoder().encodeToString(encVal);
    }

    public static String decrypt(String encryptedData, String secret) throws Exception {
        SecretKeySpec key = generateKey(secret);
        Cipher cipher = Cipher.getInstance(TRANSFORMATION);
        cipher.init(Cipher.DECRYPT_MODE, key);
        byte[] decValue = Base64.getDecoder().decode(encryptedData);
        byte[] decValueBytes = cipher.doFinal(decValue);
        return new String(decValueBytes, StandardCharsets.UTF_8);
    }

    private static SecretKeySpec generateKey(String secret) throws Exception {
        byte[] key = secret.getBytes(StandardCharsets.UTF_8);
        MessageDigest sha = MessageDigest.getInstance("SHA-256");
        key = sha.digest(key);
        key = Arrays.copyOf(key, 16); // Use only first 128 bit
        return new SecretKeySpec(key, ALGORITHM);
    }

    public static boolean validateTimestamp(long timestamp) {
        long current = System.currentTimeMillis();
        return Math.abs(current - timestamp) <= 30000; // 30 seconds
    }

    public static String getOnlineTime(String url) {
        try {
            if (new URL(url).openConnection() instanceof HttpsURLConnection connection) {
                connection.setRequestMethod("HEAD");
                long t1 = new SimpleDateFormat("EEE, dd MMM yyyy HH:mm:ss z", Locale.US).parse(connection.getHeaderField("Date")).getTime();
                long t2 = ZonedDateTime.parse(connection.getHeaderField("Date"), DateTimeFormatter.ofPattern("EEE, dd MMM yyyy HH:mm:ss z", Locale.US)).toInstant().toEpochMilli();
                if (t1 == t2) {
                    return String.valueOf(t1);
                }
            }
        } catch (Exception e) {
            Exit.exit(1);
        }
        return null;
    }

    public static String[] generateRsaKeyPair() throws Exception {
        KeyPairGenerator kpg = KeyPairGenerator.getInstance("RSA");
        kpg.initialize(2048);
        KeyPair kp = kpg.generateKeyPair();
        String publicKey = Base64.getEncoder().encodeToString(kp.getPublic().getEncoded());
        String privateKey = Base64.getEncoder().encodeToString(kp.getPrivate().getEncoded());
        return new String[]{publicKey, privateKey};
    }

    public static String encryptRsa(String data, String publicKeyStr) throws Exception {
        byte[] keyBytes = Base64.getDecoder().decode(publicKeyStr);
        X509EncodedKeySpec spec = new X509EncodedKeySpec(keyBytes);
        KeyFactory kf = KeyFactory.getInstance("RSA");
        PublicKey publicKey = kf.generatePublic(spec);
        Cipher cipher = Cipher.getInstance("RSA");
        cipher.init(Cipher.ENCRYPT_MODE, publicKey);
        byte[] encVal = cipher.doFinal(data.getBytes(StandardCharsets.UTF_8));
        return Base64.getEncoder().encodeToString(encVal);
    }

    public static String decryptRsa(String encryptedData, String privateKeyStr) throws Exception {
        byte[] keyBytes = Base64.getDecoder().decode(privateKeyStr);
        PKCS8EncodedKeySpec spec = new PKCS8EncodedKeySpec(keyBytes);
        KeyFactory kf = KeyFactory.getInstance("RSA");
        PrivateKey privateKey = kf.generatePrivate(spec);
        Cipher cipher = Cipher.getInstance("RSA");
        cipher.init(Cipher.DECRYPT_MODE, privateKey);
        byte[] decValue = Base64.getDecoder().decode(encryptedData);
        byte[] decValueBytes = cipher.doFinal(decValue);
        return new String(decValueBytes, StandardCharsets.UTF_8);
    }

    public static String signRsa(String data, String privateKeyStr) throws Exception {
        byte[] keyBytes = Base64.getDecoder().decode(privateKeyStr);
        PKCS8EncodedKeySpec spec = new PKCS8EncodedKeySpec(keyBytes);
        KeyFactory kf = KeyFactory.getInstance("RSA");
        PrivateKey privateKey = kf.generatePrivate(spec);
        Signature privateSignature = Signature.getInstance("SHA256withRSA");
        privateSignature.initSign(privateKey);
        privateSignature.update(data.getBytes(StandardCharsets.UTF_8));
        byte[] signature = privateSignature.sign();
        return Base64.getEncoder().encodeToString(signature);
    }

    public static boolean verifyRsa(String data, String signatureStr, String publicKeyStr) throws Exception {
        byte[] keyBytes = Base64.getDecoder().decode(publicKeyStr);
        X509EncodedKeySpec spec = new X509EncodedKeySpec(keyBytes);
        KeyFactory kf = KeyFactory.getInstance("RSA");
        PublicKey publicKey = kf.generatePublic(spec);
        Signature publicSignature = Signature.getInstance("SHA256withRSA");
        publicSignature.initVerify(publicKey);
        publicSignature.update(data.getBytes(StandardCharsets.UTF_8));
        byte[] signatureBytes = Base64.getDecoder().decode(signatureStr);
        return publicSignature.verify(signatureBytes);
    }
}
