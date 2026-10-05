package me.mioclient.module.misc;

import com.google.gson.JsonObject;
import java.awt.Graphics2D;
import java.awt.Image;
import java.awt.Toolkit;
import java.awt.datatransfer.ClipboardOwner;
import java.awt.datatransfer.StringSelection;
import java.awt.image.BufferedImage;
import java.awt.image.ImageObserver;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.net.URI;
import java.net.URLEncoder;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.charset.StandardCharsets;
import java.time.Duration;
import java.util.Base64;
import javax.imageio.ImageIO;
import me.mioclient.ArgumentTypeHelper;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.EnumSettingHelper;
import me.mioclient.FontsSearchHelper4;
import me.mioclient.MixinMessageIndicatorHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.SearchHelper_4;
import me.mioclient.SearchIdentifier;
import me.mioclient.TransferDataFlavorsTransferable;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.Module;
import net.minecraft.client.texture.NativeImage;
import net.minecraft.text.Text;
import net.minecraft.util.Formatting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/misc/ExtraScreenshot.class */
public class ExtraScreenshot extends Module {
    public Setting<ExtraScreenshotMode> mode;
    public Setting<Boolean> deleteFile;
    public Setting<Boolean> sound;
    public Setting<SearchIdentifier> path;
    public Setting<Float> volume;
    public final Stopwatch stopwatch;
    public int num2;
    public static final long num = 1500;
    public static final String string = "Client-ID e7c0b6a4c926098";
    public static final HttpClient httpClient = HttpClient.newHttpClient();

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/misc/ExtraScreenshot$ExtraScreenshotMode.class */
    public enum ExtraScreenshotMode implements EnumSettingHelper {
        NONE("None"),
        COPY("Copy"),
        IMGUR("Imgur");

        public final String name;

        ExtraScreenshotMode(String str) {
            this.name = str;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }
    }

    public ExtraScreenshot() {
        super("ExtraScreenshot", "Uploads your screenshots to Imgur/clipboard.", Category.MISC, new String[0]);
        PhaseESPHelper.do1351(this);
        this.stopwatch = new Stopwatch();
        this.num2 = 0;
    }

    @Override // me.mioclient.module.Module
    public String getInfo() {
        return FontsSearchHelper4.getString1684(this.mode.getValue());
    }

    @Override // me.mioclient.module.Module
    public void onEnable() {
        this.num2 = 0;
    }

    public void do2187(byte[] bArr) {
        if (this.mode.getValue() == ExtraScreenshotMode.IMGUR && !this.stopwatch.is419(1500L)) {
            this.num2++;
            this.num2 %= 3;
            MixinMessageIndicatorHelper.do345(Text.literal(String.format("Wait %dms before uploading a screenshot", Long.valueOf(1500 - this.stopwatch.get422()))).styled(style -> {
                return style.withColor(Formatting.YELLOW);
            }), MixinMessageIndicatorHelper.getMessageSignatureData337((-34596741) + this.num2), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode3);
            return;
        }
        try {
            do1016();
        } catch (Exception e) {
        }
        this.stopwatch.reset();
        if (this.mode.getValue() == ExtraScreenshotMode.COPY) {
            try {
                Image read = ImageIO.read(new ByteArrayInputStream(bArr));
                BufferedImage bufferedImage = new BufferedImage(((BufferedImage) read).getWidth(), ((BufferedImage) read).getHeight(), 1);
                Graphics2D createGraphics = bufferedImage.createGraphics();
                createGraphics.drawImage(read, 0, 0, (ImageObserver) null);
                createGraphics.dispose();
                do2189(new TransferDataFlavorsTransferable(bufferedImage));
                MixinMessageIndicatorHelper.do344(Text.of("Copied screenshot to clipboard"), MixinMessageIndicatorHelper.getMessageSignatureData339(this));
                return;
            } catch (Exception e2) {
                e2.printStackTrace();
                MixinMessageIndicatorHelper.do345(Text.literal("Failed to copy the screenshot").styled(style2 -> {
                    return style2.withColor(Formatting.RED);
                }), MixinMessageIndicatorHelper.getMessageSignatureData339(this), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode2);
                return;
            }
        }
        if (this.mode.getValue() == ExtraScreenshotMode.IMGUR) {
            MixinMessageIndicatorHelper.do344(Text.of("Uploading screenshot..."), MixinMessageIndicatorHelper.getMessageSignatureData339(this));
            try {
                JsonObject jsonObject = (JsonObject) SearchHelper_4.gson.fromJson((String) httpClient.send(HttpRequest.newBuilder().uri(new URI("https://api.imgur.com/3/image")).headers(new String[]{"Content-Type", "application/x-www-form-urlencoded"}).headers(new String[]{"Authorization", "Client-ID e7c0b6a4c926098"}).POST(HttpRequest.BodyPublishers.ofString(new ArgumentTypeHelper().getArgumentTypeHelper2919(URLEncoder.encode(Base64.getEncoder().encodeToString(bArr), StandardCharsets.UTF_8)).getString2921("image=\u0001"))).timeout(Duration.ofSeconds(10L)).build(), HttpResponse.BodyHandlers.ofString()).body(), JsonObject.class);
                if (!jsonObject.has("success") || !jsonObject.has("data")) {
                    throw new IOException("Invalid server response");
                }
                if (!jsonObject.get("success").getAsBoolean()) {
                    throw new RuntimeException("Upload failed");
                }
                JsonObject asJsonObject = jsonObject.getAsJsonObject("data");
                if (!asJsonObject.has("link")) {
                    throw new IOException("Invalid server response");
                }
                Toolkit.getDefaultToolkit().getSystemClipboard().setContents(new StringSelection(asJsonObject.get("link").getAsString()), (ClipboardOwner) null);
                MixinMessageIndicatorHelper.do344(Text.of("Copied screenshot link"), MixinMessageIndicatorHelper.getMessageSignatureData337(-43296436));
            } catch (Exception e3) {
                e3.printStackTrace();
                MixinMessageIndicatorHelper.do345(Text.literal("Failed to upload screenshot to Imgur").styled(style3 -> {
                    return style3.withColor(Formatting.RED);
                }), MixinMessageIndicatorHelper.getMessageSignatureData337(-43296435), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode2);
            }
        }
    }

    public void do2188(NativeImage nativeImage) {
        try {
            try {
                byte[] bytes = nativeImage.getBytes();
                if (is2190()) {
                    nativeImage.close();
                }
                SearchHelper_4.executorService.submit(() -> {
                    try {
                        do2187(bytes);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                });
            } catch (Exception e) {
                e.printStackTrace();
                MixinMessageIndicatorHelper.do345(Text.of("Failed to save the screenshot"), MixinMessageIndicatorHelper.getMessageSignatureData339(this), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode2);
                if (is2190()) {
                    nativeImage.close();
                }
            }
        } catch (Throwable th) {
            if (is2190()) {
                nativeImage.close();
            }
            throw th;
        }
    }

    public void do2189(TransferDataFlavorsTransferable transferDataFlavorsTransferable) {
        Toolkit.getDefaultToolkit().getSystemClipboard().setContents(transferDataFlavorsTransferable, (ClipboardOwner) null);
    }

    public void do1016() {
        if (this.sound.getValue().booleanValue()) {
            BaritoneHelper_3.searchHelper4_11.getSearchHelper4_42970(this.path.getValue()).do1820(this.volume.getValue().floatValue());
        }
    }

    public boolean is2190() {
        return this.mode.getValue() != ExtraScreenshotMode.NONE && this.deleteFile.getValue().booleanValue();
    }

    public boolean is2191() {
        return this.mode.getValue() == ExtraScreenshotMode.NONE;
    }
}
