package me.mioclient;

import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.util.concurrent.CompletableFuture;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/DiscordNotifsHelperSearchHelper4.class */
public final class DiscordNotifsHelperSearchHelper4 implements SearchHelper_4 {
    public static HttpResponse<String> getHttpResponse2959(HttpRequest httpRequest) {
        return getHttpResponse2960(httpRequest, HttpResponse.BodyHandlers.ofString());
    }

    public static <T> HttpResponse<T> getHttpResponse2960(HttpRequest httpRequest, HttpResponse.BodyHandler<T> bodyHandler) {
        try {
            return HttpClient.newHttpClient().send(httpRequest, bodyHandler);
        } catch (Throwable th) {
            throw new java.lang.RuntimeException(th);
        }
    }

    public static CompletableFuture<HttpResponse<String>> getCompletableFuture2961(HttpRequest httpRequest) {
        return getCompletableFuture2962(httpRequest, HttpResponse.BodyHandlers.ofString());
    }

    public static <T> CompletableFuture<HttpResponse<T>> getCompletableFuture2962(HttpRequest httpRequest, HttpResponse.BodyHandler<T> bodyHandler) {
        HttpClient newHttpClient = HttpClient.newHttpClient();
        return CompletableFuture.supplyAsync(() -> {
            try {
                return newHttpClient.send(httpRequest, bodyHandler);
            } catch (Throwable th) {
                throw new java.lang.RuntimeException(th);
            }
        }, executorService);
    }

    public static HttpRequest.Builder getBuilder2963(String str) {
        return getBuilder2965(str).GET();
    }

    public static HttpRequest.Builder getBuilder2964(String str, HttpRequest.BodyPublisher bodyPublisher) {
        return getBuilder2965(str).POST(bodyPublisher);
    }

    public static HttpRequest.Builder getBuilder2965(String str) {
        return HttpRequest.newBuilder().uri(URI.create(str));
    }
}
