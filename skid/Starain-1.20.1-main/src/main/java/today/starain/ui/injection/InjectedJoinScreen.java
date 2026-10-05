package today.starain.ui.injection;

import io.github.humbleui.skija.*;
import io.github.humbleui.skija.Font;
import io.github.humbleui.skija.Paint;
import io.github.humbleui.types.Rect;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.screens.ConnectScreen;
import net.minecraft.client.gui.screens.Screen;
import net.minecraft.client.multiplayer.ServerData;
import net.minecraft.client.multiplayer.resolver.ServerAddress;
import today.starain.utils.injection.ProxyInjector;
import today.starain.utils.render.skia.Skia;
import today.starain.utils.render.skia.font.Fonts;

import java.awt.Color;
import java.io.RandomAccessFile;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public final class InjectedJoinScreen {

    public enum State {INJECTING, INJECTED, JOINING, FAILED}

    private static boolean sOpen;
    private static volatile State sState = State.INJECTING;
    private static long sStateEnterMs;
    private static long sOpenAnimStartMs;
    private static long sJoiningEnterMs;
    private static Screen sParentScreen;
    private static final int OPEN_ANIM_DURATION_MS = 260;
    private static final float OPEN_SLIDE_OFFSET_PX = 32f;
    private static final List<String> sJoinLog = new ArrayList<>();

    public static boolean isOpen() {
        return sOpen;
    }

    public static void close() {
        sOpen = false;
    }

    public static void setParent(Screen s) {
        sParentScreen = s;
    }

    public static State getState() {
        return sState;
    }

    public static boolean isJoining() {
        return sOpen && sState == State.JOINING;
    }

    public static void open() {
        sOpen = true;
        sOpenAnimStartMs = System.currentTimeMillis();

        if (ProxyInjector.isProxyListening()) {
            ProxyInjector.markInjectedIfListening();
            enter(State.INJECTED);
            return;
        }
        enter(State.INJECTING);
        new Thread(() -> {
            try {
                Thread.sleep(350);
                boolean loaded = ProxyInjector.tryInject();
                if (!loaded) {
                    enter(State.FAILED);
                    return;
                }

                enter(State.INJECTED);
            } catch (Throwable t) {
                ProxyInjector.markFailed(t.getClass().getSimpleName() + ": " + t.getMessage());
                enter(State.FAILED);
            }
        }, "starain-injector").start();
    }

    private static void enter(State s) {
        sState = s;
        sStateEnterMs = System.currentTimeMillis();
        if (s == State.JOINING) {
            sJoiningEnterMs = System.currentTimeMillis();
            sJoinLog.clear();
            new Thread(InjectedJoinScreen::runJoinAnimation, "starain-join-anim").start();
        }
    }

    private static float easeOut(long delay, long duration) {
        long e = System.currentTimeMillis() - sJoiningEnterMs - delay;
        if (e <= 0) return 0f;
        if (e >= duration) return 1f;
        float x = (float) e / duration;
        return 1f - (float) Math.pow(1f - x, 3);
    }

    private static Thread sLogTailThread;
    private static volatile boolean sLogTailRunning;

    private static void runJoinAnimation() {
        startLogTail();
        sleep(3000);

        Minecraft mc = Minecraft.getInstance();
        String addr = "127.0.0.1:" + ProxyInjector.PROXY_PORT;
        Screen parent = sParentScreen != null ? sParentScreen : mc.screen;

        mc.execute(() -> {
            try {
                ServerData sd = new ServerData("Starain Proxy", addr, false);
                ConnectScreen.startConnecting(parent, mc,
                        ServerAddress.parseString(addr), sd, false);
            } catch (Throwable t) {
                ProxyInjector.markFailed("connect: " + t.getMessage());
            }
        });

        long appearDeadline = System.currentTimeMillis() + 3000;
        while (System.currentTimeMillis() < appearDeadline
                && !(mc.screen instanceof ConnectScreen)) sleep(80);

        sleep(1500);

        mc.execute(() -> {
            try {
                if (mc.screen instanceof ConnectScreen cs) {
                    for (java.lang.reflect.Field f : ConnectScreen.class.getDeclaredFields()) {
                        if (f.getType() == net.minecraft.network.Connection.class) {
                            f.setAccessible(true);
                            net.minecraft.network.Connection conn =
                                    (net.minecraft.network.Connection) f.get(cs);
                            if (conn != null) {
                                for (java.lang.reflect.Field cf : net.minecraft.network.Connection.class.getDeclaredFields()) {
                                    if (cf.getType() == io.netty.channel.Channel.class) {
                                        cf.setAccessible(true);
                                        io.netty.channel.Channel ch = (io.netty.channel.Channel) cf.get(conn);
                                        if (ch != null && ch.isOpen()) ch.close();
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                    }
                }
            } catch (Throwable ignored) {
            }
        });

        sleep(700);

        mc.execute(() -> {
            try {
                ServerData sd = new ServerData("Starain Proxy", addr, false);
                ConnectScreen.startConnecting(parent, mc,
                        ServerAddress.parseString(addr), sd, false);
            } catch (Throwable t) {
                ProxyInjector.markFailed("reconnect: " + t.getMessage());
            }
        });

        sleep(500);
        stopLogTail();
        close();
    }

    private static void startLogTail() {
        stopLogTail();
        sLogTailRunning = true;
        sLogTailThread = new Thread(() -> {
            try {
                java.nio.file.Path logFile = java.nio.file.Path.of(
                        Minecraft.getInstance().gameDirectory.getAbsolutePath(),
                        "logs", "latest.log");
                if (!Files.exists(logFile)) return;

                List<String> all = Files.readAllLines(logFile, StandardCharsets.UTF_8);
                List<String> seed = new ArrayList<>();
                for (int i = all.size() - 1; i >= 0 && seed.size() < 20; i--) {
                    String s = all.get(i);
                    if (isNoise(s)) continue;
                    seed.add(0, s);
                }
                for (String s : seed) appendLog(clip(s));

                try (RandomAccessFile raf = new RandomAccessFile(logFile.toFile(), "r")) {
                    raf.seek(raf.length());
                    while (sLogTailRunning) {
                        String line = raf.readLine();
                        if (line == null) {
                            Thread.sleep(120);
                        } else {
                            String utf = new String(line.getBytes(StandardCharsets.ISO_8859_1),
                                    StandardCharsets.UTF_8);
                            if (!isNoise(utf)) appendLog(clip(utf));
                        }
                    }
                }
            } catch (Throwable ignored) {
            }
        }, "starain-logtail");
        sLogTailThread.setDaemon(true);
        sLogTailThread.start();
    }

    private static boolean isNoise(String s) {
        if (s == null) return true;
        String t = s.trim();
        if (t.isEmpty()) return true;

        if (t.startsWith("at ")) return true;
        if (t.startsWith("Caused by:")) return true;
        if (t.startsWith("... ") && t.endsWith(" more")) return true;

        if (t.matches("^[a-zA-Z_][\\w.$]*Exception[:\\s].*")) return true;
        if (t.matches("^[a-zA-Z_][\\w.$]*Error[:\\s].*")) return true;

        if (t.contains("/WARN]")) return true;
        if (t.contains("/ERROR]")) return true;
        if (t.contains("/FATAL]")) return true;
        if (t.contains("/DEBUG]")) return true;
        if (t.contains("/TRACE]")) return true;

        if (t.contains("OpenGL debug message")) return true;
        if (t.contains("GL_INVALID")) return true;
        if (t.contains("[STDERR]")) return true;
        if (t.contains("Failed to dispatch event")) return true;
        if (t.contains("InvocationTargetException")) return true;
        if (t.contains("does not have attribute")) return true;
        if (t.contains("Received packet for unknown team")) return true;
        if (t.contains("has been tampered with")) return true;
        return false;
    }

    private static void stopLogTail() {
        sLogTailRunning = false;
        if (sLogTailThread != null) {
            sLogTailThread.interrupt();
            sLogTailThread = null;
        }
    }

    private static String clip(String s) {
        if (s == null) return "";
        return s.length() <= 130 ? s : s.substring(0, 127) + "...";
    }

    private static void appendLog(String line) {
        synchronized (sJoinLog) {
            sJoinLog.add(line);
            if (sJoinLog.size() > 80) sJoinLog.remove(0);
        }
    }

    private static void sleep(long ms) {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException ignored) {
        }
    }

    private static final Color OVERLAY = new Color(0, 0, 0, 190);
    private static final Color JOIN_BG = new Color(12, 18, 30, 240);
    private static final Color PANEL_BG = new Color(14, 20, 34, 255);
    private static final Color CARD_BG = new Color(22, 28, 44, 255);
    private static final Color CYAN = new Color(88, 220, 235, 255);
    private static final Color CYAN_DIM = new Color(88, 220, 235, 200);
    private static final Color BTN_BG = new Color(74, 82, 96, 255);
    private static final Color BTN_HOV = new Color(100, 110, 128, 255);
    private static final Color LOG_INFO = new Color(200, 220, 230, 255);
    private static final Color LOG_LABEL = new Color(170, 175, 190, 255);
    private static final Color LABEL_C = new Color(150, 155, 170, 255);
    private static final Color TIP_TEXT = new Color(170, 175, 190, 255);
    private static final Color ERR_TEXT = new Color(240, 130, 130, 255);

    private static float sPanelX, sPanelY, sPanelW, sPanelH;
    private static float sBtnX, sBtnY, sBtnW, sBtnH;

    public static void render(int mouseX, int mouseY) {
        if (!sOpen) return;
        if (sState == State.JOINING) return;

        Minecraft mc = Minecraft.getInstance();
        int sw = mc.getWindow().getGuiScaledWidth();
        int sh = mc.getWindow().getGuiScaledHeight();

        long elapsed = System.currentTimeMillis() - sOpenAnimStartMs;
        float t = elapsed >= OPEN_ANIM_DURATION_MS ? 1f
                : (float) elapsed / OPEN_ANIM_DURATION_MS;
        float eased = 1f - (float) Math.pow(1f - t, 3);
        float slideDy = (1f - eased) * OPEN_SLIDE_OFFSET_PX;

        int overlayA = (int) (OVERLAY.getAlpha() * eased);
        Skia.drawRect(0, 0, sw, sh, new Color(0, 0, 0, overlayA));

        final float pw = 360f, ph = 240f;
        final float px = (sw - pw) / 2f, py = (sh - ph) / 2f;
        sPanelX = px;
        sPanelY = py;
        sPanelW = pw;
        sPanelH = ph;

        Skia.save();
        try {
            Skia.getCanvas().translate(0f, slideDy);
            Skia.drawRoundedRect(px, py, pw, ph, 12f, PANEL_BG);

            Font titleFont = Fonts.getInterBold(13.5f);
            String title = "Starain™";
            float tw = Skia.getTextWidth(title, titleFont);
            Skia.drawText(title, px + (pw - tw) / 2f, py + 16f, Color.WHITE, titleFont);
            Skia.drawRect(px + 20f, py + 38f, pw - 40f, 1.2f, CYAN);

            switch (sState) {
                case INJECTING -> renderInjecting(px, py, pw, ph);
                case INJECTED -> renderInjected(px, py, pw, ph, mouseX, mouseY);
                case FAILED -> renderFailed(px, py, pw, ph, mouseX, mouseY);
                default -> {
                }
            }
        } finally {
            Skia.restore();
        }
    }

    public static void renderJoiningOverlay(int mouseX, int mouseY) {
        if (!sOpen || sState != State.JOINING) return;

        Minecraft mc = Minecraft.getInstance();

        if (mc.screen == null && mc.level != null && mc.player != null) {
            close();
            return;
        }

        int sw = mc.getWindow().getGuiScaledWidth();
        int sh = mc.getWindow().getGuiScaledHeight();
        renderJoiningFullScreen(sw, sh);
    }

    private static void renderJoiningFullScreen(int sw, int sh) {

        float bgT = easeOut(0, 260);
        Skia.drawRect(0, 0, sw, sh,
                new Color(JOIN_BG.getRed(), JOIN_BG.getGreen(), JOIN_BG.getBlue(),
                        (int) (JOIN_BG.getAlpha() * bgT)));

        float tServerCard = easeOut(0, 380);
        float tProfileCard = easeOut(90, 380);
        float tLogo = easeOut(180, 380);
        float tSpinner = easeOut(270, 320);
        float tJoinText = easeOut(360, 380);

        Minecraft mc = Minecraft.getInstance();

        Font cardTitleFont = Fonts.getInterBold(9.5f);
        Font cardValueFont = Fonts.getInterRegular(9.5f);
        float cardW = 220f;
        float cardH = 44f;
        float cardBaseX = 10f;
        float cardBaseY = 10f;
        {
            float sx = -cardW - 20f;
            float x = cardBaseX + (sx - cardBaseX) * (1f - tServerCard) + (cardBaseX - sx) * 0f;

            x = cardBaseX + (1f - tServerCard) * (sx - cardBaseX);
            drawCardAlpha(x, cardBaseY, cardW, cardH, tServerCard,
                    "Server Information",
                    "Address: 127.0.0.1:" + ProxyInjector.PROXY_PORT,
                    true, cardTitleFont, cardValueFont);
        }

        {
            float cardBaseY2 = cardBaseY + cardH + 8f;
            float sx = -cardW - 20f;
            float x = cardBaseX + (1f - tProfileCard) * (sx - cardBaseX);
            drawCardAlpha(x, cardBaseY2, cardW, cardH, tProfileCard,
                    "Profile Information",
                    "Username: " + safe(mc.getUser().getName()),
                    false, cardTitleFont, cardValueFont);
        }

        float cx = sw / 2f;
        float cy = sh / 2f - 40f;
        {
            Font logoFont = Fonts.getInterBold(48f);
            String logo = "S";
            float lw = Skia.getTextWidth(logo, logoFont);
            float baseY = cy - 30f;
            float y = baseY - 80f * (1f - tLogo);
            int a = (int) (255 * tLogo);
            if (a > 4) {
                Skia.drawText(logo, cx - lw / 2f, y, new Color(255, 255, 255, a), logoFont);
            }
        }

        if (tSpinner > 0.02f) {
            drawSpinnerAlpha(cx, cy + 40f, 10f, tSpinner);
        }

        {
            Font titleF = Fonts.getInterMedium(12f);
            String txt = "Joining Server";
            float tw2 = Skia.getTextWidth(txt, titleF);
            float baseY = cy + 58f;
            float y = baseY + 40f * (1f - tJoinText);
            int a = (int) (255 * tJoinText);
            if (a > 4) {
                Skia.drawText(txt, cx - tw2 / 2f, y, new Color(255, 255, 255, a), titleF);
            }
        }

        Font logFont = Fonts.getNormal(8f);
        float logX = 10f;
        float rowH = 10f;
        float bottomY = sh - 12f;
        float fadeStart = sh * 0.55f;
        float fadeEnd = sh * 0.32f;
        synchronized (sJoinLog) {
            int n = sJoinLog.size();
            for (int i = 0; i < n; i++) {
                String line = sJoinLog.get(n - 1 - i);
                float y = bottomY - i * rowH;
                if (y < fadeEnd) break;
                float alpha01;
                if (y >= fadeStart) alpha01 = 1f;
                else alpha01 = (y - fadeEnd) / (fadeStart - fadeEnd);
                Color base = colorForLog(line);
                int a = (int) (base.getAlpha() * alpha01);
                if (a <= 4) continue;
                Skia.drawText(line, logX, y,
                        new Color(base.getRed(), base.getGreen(), base.getBlue(), a),
                        logFont);
            }
        }

        sBtnX = 0;
        sBtnY = 0;
        sBtnW = 0;
        sBtnH = 0;
    }

    private static Color colorForLog(String line) {

        if (line.startsWith("Title:") || line.startsWith("PID:")
                || line.startsWith("Handle:") || line.startsWith("Class:")) {
            return LOG_LABEL;
        }
        if (line.startsWith("=====")) return LOG_LABEL;
        if (line.startsWith("Done.")) return CYAN_DIM;
        if (line.startsWith("ERROR")) return ERR_TEXT;
        return LOG_INFO;
    }

    private static void renderInjecting(float px, float py, float pw, float ph) {
        float cx = px + pw / 2f;
        float cy = py + ph / 2f - 6f;
        drawSpinner(cx, cy, 12f);

        Font f = Fonts.getInterMedium(11f);
        Font zh = Fonts.getMedium(11f);
        String eng = "Injecting into ";
        String cn = "布吉岛…";
        float ew = Skia.getTextWidth(eng, f);
        float cw = Skia.getTextWidth(cn, zh);
        float total = ew + cw;
        Skia.drawText(eng, cx - total / 2f, cy + 26f, Color.WHITE, f);
        Skia.drawText(cn, cx - total / 2f + ew, cy + 26f, Color.WHITE, zh);

        long pid = ProxyInjector.lastBujiPid();
        if (pid > 0) {
            Font sub = Fonts.getInterRegular(9f);
            String s = "PID " + pid;
            float w = Skia.getTextWidth(s, sub);
            Skia.drawText(s, cx - w / 2f, cy + 44f, TIP_TEXT, sub);
        }
        sBtnX = 0;
        sBtnY = 0;
        sBtnW = 0;
        sBtnH = 0;
    }

    private static void renderInjected(float px, float py, float pw, float ph,
                                       int mouseX, int mouseY) {
        Font labelFont = Fonts.getInterBold(10.5f);
        Font valueFont = Fonts.getInterRegular(10.5f);
        float rowY = py + 60f;
        float labelX = px + 24f;

        Minecraft mc = Minecraft.getInstance();
        String uname = safe(mc.getUser().getName());
        UUID uuid = mc.getUser().getProfileId();

        drawRow(labelX, rowY, "Server:", "127.0.0.1:" + ProxyInjector.PROXY_PORT, labelFont, valueFont);
        drawRow(labelX, rowY + 24, "Version:", "1.20.1", labelFont, valueFont);
        drawRow(labelX, rowY + 48, "Username:", uname, labelFont, valueFont);
        drawRow(labelX, rowY + 72, "UUID:", uuid != null ? uuid.toString() : "-", labelFont, valueFont);

        drawPillButton("Join", px, py, pw, ph, mouseX, mouseY);
    }

    private static void renderFailed(float px, float py, float pw, float ph,
                                     int mouseX, int mouseY) {
        String reason = ProxyInjector.failReason();
        if (reason == null) reason = "Injection failed";

        float cx = px + pw / 2f;
        float cy = py + 80f;
        String line1 = "Injection failed";
        Font bf = Fonts.getInterBold(11f);
        float w1 = Skia.getTextWidth(line1, bf);
        Skia.drawText(line1, cx - w1 / 2f, cy, ERR_TEXT, bf);

        String[] lines = wrap(reason, 52);
        Font tipFont = Fonts.getMedium(9.5f);
        for (int i = 0; i < lines.length && i < 3; i++) {
            float lw = Skia.getTextWidth(lines[i], tipFont);
            Skia.drawText(lines[i], cx - lw / 2f, cy + 20f + i * 14f, TIP_TEXT, tipFont);
        }
        drawPillButton("Retry", px, py, pw, ph, mouseX, mouseY);
    }

    private static void drawPillButton(String text, float px, float py, float pw, float ph,
                                       int mouseX, int mouseY) {
        final float bh = 28f;
        final float bw = 100f;
        final float bx = px + (pw - bw) / 2f;
        final float by = py + ph - bh - 20f;
        sBtnX = bx;
        sBtnY = by;
        sBtnW = bw;
        sBtnH = bh;

        boolean hover = inRect(mouseX, mouseY, bx, by, bw, bh);
        Skia.drawRoundedRect(bx, by, bw, bh, bh / 2f, hover ? BTN_HOV : BTN_BG);

        Font btnFont = Fonts.getInterBold(10.5f);
        float tw = Skia.getTextWidth(text, btnFont);
        Skia.drawText(text, bx + (bw - tw) / 2f, by + (bh - 10.5f) / 2f - 0.5f,
                Color.WHITE, btnFont);
    }

    private static void drawCardAlpha(float x, float y, float w, float h, float alpha01,
                                      String title, String value, boolean serverIcon,
                                      Font titleFont, Font valueFont) {
        if (alpha01 <= 0.02f) return;
        int a = (int) (255 * alpha01);
        Color bg = new Color(CARD_BG.getRed(), CARD_BG.getGreen(), CARD_BG.getBlue(),
                (int) (CARD_BG.getAlpha() * alpha01));
        Skia.drawRoundedRect(x, y, w, h, 8f, bg);

        Color iconC = new Color(CYAN.getRed(), CYAN.getGreen(), CYAN.getBlue(), a);
        if (serverIcon) {
            Skia.drawRoundedRect(x + 8f, y + 15f, 14f, 3.5f, 1f, iconC);
            Skia.drawRoundedRect(x + 8f, y + 15f + 5f, 14f, 3.5f, 1f, iconC);
            Skia.drawRoundedRect(x + 8f, y + 15f + 10f, 14f, 3.5f, 1f, iconC);
        } else {
            Color d = new Color(LOG_INFO.getRed(), LOG_INFO.getGreen(), LOG_INFO.getBlue(), a);
            Skia.drawRoundedRect(x + 8f, y + 14f, 3f, 3f, 1f, iconC);
            Skia.drawRoundedRect(x + 14f, y + 14f, 8f, 3f, 1f, d);
            Skia.drawRoundedRect(x + 8f, y + 14f + 5f, 3f, 3f, 1f, iconC);
            Skia.drawRoundedRect(x + 14f, y + 14f + 5f, 8f, 3f, 1f, d);
            Skia.drawRoundedRect(x + 8f, y + 14f + 10f, 3f, 3f, 1f, iconC);
            Skia.drawRoundedRect(x + 14f, y + 14f + 10f, 8f, 3f, 1f, d);
        }
        Color labelC = new Color(LABEL_C.getRed(), LABEL_C.getGreen(), LABEL_C.getBlue(), a);
        Color whiteC = new Color(255, 255, 255, a);
        Skia.drawText(title, x + 30f, y + 8f, labelC, titleFont);
        Skia.drawText(value, x + 30f, y + 22f, whiteC, valueFont);
    }

    private static void drawSpinnerAlpha(float cx, float cy, float r, float alpha01) {
        Paint paint = null;
        Path path = null;
        try {
            paint = new Paint();
            paint.setAntiAlias(true);
            paint.setMode(PaintMode.STROKE);
            paint.setStrokeWidth(2.4f);
            paint.setStrokeCap(PaintStrokeCap.ROUND);
            paint.setARGB((int) (255 * alpha01), CYAN.getRed(), CYAN.getGreen(), CYAN.getBlue());
            path = new Path();
            float startAngle = (System.currentTimeMillis() % 1200L) / 1200f * 360f;
            path.addArc(Rect.makeXYWH(cx - r, cy - r, r * 2f, r * 2f), startAngle, 260f);
            Skia.getCanvas().drawPath(path, paint);
        } finally {
            if (path != null) path.close();
            if (paint != null) paint.close();
        }
    }

    private static void drawSpinner(float cx, float cy, float r) {
        Paint paint = null;
        Path path = null;
        try {
            paint = new Paint();
            paint.setAntiAlias(true);
            paint.setMode(PaintMode.STROKE);
            paint.setStrokeWidth(2.4f);
            paint.setStrokeCap(PaintStrokeCap.ROUND);
            paint.setARGB(255, CYAN.getRed(), CYAN.getGreen(), CYAN.getBlue());
            path = new Path();
            float startAngle = (System.currentTimeMillis() % 1200L) / 1200f * 360f;
            path.addArc(Rect.makeXYWH(cx - r, cy - r, r * 2f, r * 2f),
                    startAngle, 260f);
            Skia.getCanvas().drawPath(path, paint);
        } finally {
            if (path != null) path.close();
            if (paint != null) paint.close();
        }
    }

    private static void drawServerIcon(float x, float y) {
        Skia.drawRoundedRect(x, y, 14f, 3.5f, 1f, CYAN);
        Skia.drawRoundedRect(x, y + 5f, 14f, 3.5f, 1f, CYAN);
        Skia.drawRoundedRect(x, y + 10f, 14f, 3.5f, 1f, CYAN);
    }

    private static void drawListIcon(float x, float y) {
        Skia.drawRoundedRect(x, y, 3f, 3f, 1f, CYAN);
        Skia.drawRoundedRect(x + 6f, y, 8f, 3f, 1f, LOG_INFO);
        Skia.drawRoundedRect(x, y + 5f, 3f, 3f, 1f, CYAN);
        Skia.drawRoundedRect(x + 6f, y + 5f, 8f, 3f, 1f, LOG_INFO);
        Skia.drawRoundedRect(x, y + 10f, 3f, 3f, 1f, CYAN);
        Skia.drawRoundedRect(x + 6f, y + 10f, 8f, 3f, 1f, LOG_INFO);
    }

    public static boolean handleClick(double mx, double my) {
        if (!sOpen) return false;
        if (sState == State.JOINING) return true;
        if (sBtnW > 0 && inRect(mx, my, sBtnX, sBtnY, sBtnW, sBtnH)) {
            switch (sState) {
                case INJECTED -> enter(State.JOINING);
                case FAILED -> open();
                default -> {
                }
            }
        }
        return true;
    }

    private static void drawRow(float x, float y, String label, String value,
                                Font labelFont, Font valueFont) {
        Skia.drawText(label, x, y, Color.WHITE, labelFont);
        float lw = Skia.getTextWidth(label, labelFont);
        Skia.drawText(value, x + lw + 6f, y, CYAN, valueFont);
    }

    private static boolean inRect(double mx, double my, float x, float y, float w, float h) {
        return mx >= x && mx <= x + w && my >= y && my <= y + h;
    }

    private static String safe(String s) {
        return s == null ? "-" : s;
    }

    private static String[] wrap(String s, int cols) {
        if (s == null) return new String[0];
        List<String> out = new ArrayList<>();
        while (s.length() > cols) {
            out.add(s.substring(0, cols));
            s = s.substring(cols);
        }
        if (!s.isEmpty()) out.add(s);
        return out.toArray(new String[0]);
    }

    private InjectedJoinScreen() {
    }
}
