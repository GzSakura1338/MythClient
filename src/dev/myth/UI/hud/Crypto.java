package dev.myth.UI.hud;

import com.google.gson.JsonParser;
import dev.myth.UI.setting.Setting;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.util.List;
import java.util.Locale;

public final class Crypto extends TextHudElement {
    public final Setting<String> coin = addSetting(Setting.text("coin", "Coin", "BTC").onChange(ignored -> reset()));
    public final Setting<String> fiat = addSetting(Setting.choice("fiat", "Fiat", "USD", List.of("USD", "EUR", "RUB", "CNY", "TRY", "JPY", "PLN", "BRL")).onChange(ignored -> reset()));
    private final HttpClient http = HttpClient.newBuilder().connectTimeout(Duration.ofSeconds(10)).build();
    private volatile String price = "0.00", formatting = "\u00a7f";
    private volatile boolean pending;
    private long lastRefresh;
    private volatile long generation;
    public Crypto() { super("crypto", "Crypto"); }
    @Override public String getDescription() { return "Displays the selected cryptocurrency price, refreshed every five seconds."; }
    private void reset() { generation++; price = "0.00"; formatting = "\u00a7f"; lastRefresh = 0; }
    @Override protected String getText() { return coin.get().toUpperCase(Locale.ROOT) + " " + formatting + price; }
    @Override protected void onTick() {
        if (pending || System.currentTimeMillis() - lastRefresh < 5000) return;
        String symbol = coin.get().toUpperCase(Locale.ROOT), currency = fiat.get();
        if (!symbol.matches("[A-Z]+")) { price = "0.00"; formatting = "\u00a7f"; return; }
        lastRefresh = System.currentTimeMillis(); pending = true; long requestGeneration = generation;
        http.sendAsync(HttpRequest.newBuilder(URI.create("https://api.coinconvert.net/convert/" + symbol + "/" + currency + "?amount=1"))
                .timeout(Duration.ofSeconds(10)).header("Accept", "application/json").header("User-Agent", "Myth/1.0").GET().build(), HttpResponse.BodyHandlers.ofString())
                .whenComplete((response, error) -> {
                    try {
                        if (requestGeneration != generation || !isEnabled()) return;
                        if (error != null || response.statusCode() != 200) { price = "UNKNOWN"; formatting = "\u00a7f"; return; }
                        var json = JsonParser.parseString(response.body()).getAsJsonObject();
                        if (!json.has("status") || !json.has(currency)) { price = "UNKNOWN"; return; }
                        String next = String.format(Locale.ROOT, "%.2f", json.get(currency).getAsFloat());
                        float change = (price.equals("0.00") || price.equals("UNKNOWN")) ? 0 : Float.parseFloat(next) - Float.parseFloat(price);
                        formatting = Math.abs(change) < .01 ? "\u00a7f" : change < 0 ? "\u00a7c" : "\u00a7a"; price = next;
                    } catch (RuntimeException ignored) { price = "UNKNOWN"; formatting = "\u00a7f"; }
                    finally { pending = false; }
                });
    }
    @Override public void close() { generation++; http.shutdownNow(); }
}
