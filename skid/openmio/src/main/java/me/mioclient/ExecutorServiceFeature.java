package me.mioclient;

import com.google.gson.JsonObject;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.regex.Pattern;
import net.minecraft.command.CommandSource;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ExecutorServiceFeature.class */
public class ExecutorServiceFeature extends Feature {
    public final ExecutorService executorService;
    public final HttpClient httpClient;
    public static final Pattern pattern = Pattern.compile("[a-zA-Z0-9_]{1,16}");

    public ExecutorServiceFeature(String str) {
        super(str);
        this.executorService = Executors.newCachedThreadPool();
        this.httpClient = HttpClient.newHttpClient();
    }

    @Override // me.mioclient.Feature
    public void exec(com.mojang.brigadier.builder.LiteralArgumentBuilder<CommandSource> literalArgumentBuilder) {
    }

    public CompletableFuture<JsonObject> getCompletableFuture1449(String str, String str2) {
        CompletableFuture<JsonObject> completableFuture = new CompletableFuture<>();
        this.executorService.submit(() -> {
            int length;
            if (str2 != null && !str2.isEmpty() && ((length = str2.length()) < 3 || length > 16 || !pattern.matcher(str2).matches())) {
                completableFuture.completeExceptionally(new java.lang.RuntimeException("Invalid username provided."));
                return;
            }
            String str3 = str;
            if (str2 != null && !str2.isEmpty()) {
                str3 = new ArgumentTypeHelper().getArgumentTypeHelper2919(str2).getArgumentTypeHelper2919(new ArgumentTypeHelper().getArgumentTypeHelper2919(str3).getString2921("\u0001?playerName=")).getString2921("\u0001\u0001");
            }
            for (int i = 0; i < 2; i++) {
                try {
                    HttpResponse send = this.httpClient.send(HttpRequest.newBuilder().uri(new URI(new ArgumentTypeHelper().getArgumentTypeHelper2919(str3).getString2921("https://api.2b2t.vc/\u0001"))).GET().header("User-Agent", "MioClient/2.0").header("Accept", "application/json").timeout(Duration.ofSeconds(10L)).build(), HttpResponse.BodyHandlers.ofString());
                    if (send.statusCode() == 204) {
                        Object[] objArr = new Object[1];
                        objArr[0] = (str2 == null || str2.isEmpty()) ? "" : " for player";
                        completableFuture.completeExceptionally(new java.lang.RuntimeException("No data%s.".formatted(objArr)));
                        return;
                    } else if (send.statusCode() == 429) {
                        completableFuture.completeExceptionally(new java.lang.RuntimeException("You are being rate limited, please try again later."));
                        return;
                    } else if (send.statusCode() != 200) {
                        completableFuture.completeExceptionally(new java.lang.RuntimeException("Invalid server response code: %d.".formatted(Integer.valueOf(send.statusCode()))));
                        return;
                    } else {
                        completableFuture.complete((JsonObject) gson.fromJson((String) send.body(), JsonObject.class));
                        return;
                    }
                } catch (Exception e) {
                    try {
                        Thread.sleep(1000L);
                    } catch (Exception e2) {
                    }
                }
            }
            if (0 == 0) {
                completableFuture.completeExceptionally(new java.lang.RuntimeException("Failed to fetch data."));
            }
        });
        return completableFuture;
    }

    public static String getString1450(int i) {
        StringBuilder sb = new StringBuilder();
        int i2 = i / 86400;
        int i3 = (i / 3600) % 24;
        int i4 = (i / 60) % 60;
        int i5 = i % 60;
        if (i2 > 0) {
            sb.append(i2);
            sb.append(" day%s, ".formatted(getString1451(i2)));
        }
        if (i2 > 0 || i3 > 0) {
            sb.append(i3);
            sb.append(" hour%s, ".formatted(getString1451(i3)));
        }
        if (i2 > 0 || i3 > 0 || i4 > 0) {
            sb.append(i4);
            sb.append(" minute%s and ".formatted(getString1451(i4)));
        }
        sb.append(i5);
        sb.append(" second%s".formatted(getString1451(i5)));
        return sb.toString();
    }

    public static String getString1451(int i) {
        return i == 1 ? "" : "s";
    }
}
