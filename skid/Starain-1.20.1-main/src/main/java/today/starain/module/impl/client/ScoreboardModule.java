package today.starain.module.impl.client;

import com.google.common.collect.Iterables;
import com.google.common.collect.Lists;
import com.mojang.datafixers.util.Pair;
import io.github.humbleui.skija.Font;
import net.minecraft.network.chat.Component;
import net.minecraft.network.chat.MutableComponent;
import net.minecraft.network.chat.Style;
import net.minecraft.network.chat.TextColor;
import net.minecraft.client.gui.GuiGraphics;
import net.minecraft.world.scores.Objective;
import net.minecraft.world.scores.PlayerTeam;
import net.minecraft.world.scores.Score;
import net.minecraft.world.scores.Scoreboard;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import today.starain.Starain;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.RenderSkiaEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.ui.Design;
import today.starain.ui.Theme;
import today.starain.utils.render.skia.Skia;
import today.starain.utils.render.skia.font.Fonts;
import today.starain.value.impl.BooleanValue;
import today.starain.value.impl.NumberValue;

import java.awt.*;
import java.util.Collection;
import java.util.List;
import java.util.stream.Collectors;

public class ScoreboardModule extends ClientModule {

    private static final String IP_TEXT = "www.starain.today";
    private static Objective objective;
    private static int vanillaLineCounter;
    // Custom Render 关掉后走原版计分板，X/Y/Render Score 只对自绘模式生效，靠可见性隐藏。
    private final BooleanValue customRender = new BooleanValue("Custom Render", true, this);
    private final NumberValue x = new NumberValue("X", 0, 5000, 10, 1, this, customRender::get);
    private final NumberValue y = new NumberValue("y", 0, 1000, 150, 1, this, customRender::get);
    private final BooleanValue renderScore = new BooleanValue("Render Score", false, this, customRender::get);
    private final BooleanValue replaceIP = new BooleanValue("Replace Server IP", true, this);

    public ScoreboardModule() {
        setName("ScoreboardModule");
        setCategory(Category.CLIENT);
    }

    public static void renderSB(GuiGraphics guiGraphics, Objective objective, CallbackInfo ci) {
        ScoreboardModule sbModule = Starain.getInstance().getModuleManager().getModule(ScoreboardModule.class);
        if (sbModule == null || !sbModule.isEnabled()) {
            ScoreboardModule.objective = null;
            return;
        }
        if (sbModule.customRender.get()) {
            // 自绘重模式：接管整块渲染。
            ScoreboardModule.objective = objective;
            ci.cancel();
        } else {
            // 原版模式：不取消，让 vanilla 画；IP 行由 MixinGui 的 drawString @Redirect 替换。
            ScoreboardModule.objective = null;
        }
    }

    /** 每次 displayScoreboardSidebar 开始时重置行计数（MixinGui HEAD 调用）。 */
    public static void resetVanillaLineCounter() {
        vanillaLineCounter = 0;
    }

    /**
     * 原版模式下每个名字/标题行 drawString 的重定向目标。第一行（底行，与自绘模式替换的
     * scoreDisplayData.get(0) 一致）在开启 Replace Server IP 时换成流动渐变的 IP 文本，
     * 用传入的 vanilla font 绘制——与其它计分板行字体一致。
     */
    public static int drawVanillaLine(GuiGraphics guiGraphics, net.minecraft.client.gui.Font font,
                                      Component text, int lineX, int lineY, int color, boolean shadow) {
        int index = vanillaLineCounter++;
        ScoreboardModule sbModule = Starain.getInstance().getModuleManager().getModule(ScoreboardModule.class);
        if (index == 0 && sbModule != null && sbModule.isEnabled()
                && !sbModule.customRender.get() && sbModule.replaceIP.get()) {
            return guiGraphics.drawString(font, buildFlowingIp(), lineX, lineY, color, shadow);
        }
        return guiGraphics.drawString(font, text, lineX, lineY, color, shadow);
    }

    /** 逐字用 Theme.accent(-i) 上色，复刻自绘模式 Skia.drawGradientText 的主题流动效果。 */
    private static Component buildFlowingIp() {
        MutableComponent component = Component.empty();
        for (int i = 0; i < IP_TEXT.length(); i++) {
            java.awt.Color c = Theme.accent(-i);
            component.append(Component.literal(String.valueOf(IP_TEXT.charAt(i)))
                    .setStyle(Style.EMPTY.withColor(TextColor.fromRgb(c.getRGB() & 0xFFFFFF))));
        }
        return component;
    }

    @EventTarget
    public void onRender(RenderSkiaEvent event) {
        if (objective == null) return;

        try {
            Scoreboard scoreboard = objective.getScoreboard();
            Collection<Score> allScores = scoreboard.getPlayerScores(objective);

            List<Score> validScores = allScores.stream()
                    .filter(score -> score.getOwner() != null && !score.getOwner().startsWith("#"))
                    .collect(Collectors.toList());
            List<Score> visibleScores;
            if (validScores.size() > 15) {
                visibleScores = Lists.newArrayList(Iterables.skip(validScores, validScores.size() - 15));
            } else {
                visibleScores = validScores;
            }
            List<Pair<Score, Component>> scoreDisplayData = Lists.newArrayListWithCapacity(visibleScores.size());

            Component titleComponent = objective.getDisplayName();
            Font font = Fonts.getNormal(7);
            float titleWidth = Skia.getTextWidth(Skia.getLegacyString(titleComponent), font);
            float maxLineWidth = Math.max(replaceIP.get() ? Skia.getTextWidth("www.starain.today", font) : 0, titleWidth);
            float colonWidth = Skia.getTextWidth(": ", font);

            for (Score score : visibleScores) {
                PlayerTeam playerTeam = scoreboard.getPlayersTeam(score.getOwner());
                MutableComponent ownerName = PlayerTeam.formatNameForTeam(playerTeam, Component.literal(score.getOwner()));
                scoreDisplayData.add(Pair.of(score, ownerName));

                maxLineWidth = Math.max(maxLineWidth, Skia.getTextWidth(Skia.getLegacyString(ownerName), font) + colonWidth + Skia.getTextWidth(Integer.toString(score.getScore()), font));
            }

            float lineHeight = 9.5f;
            float titleGap = 2f;
            float bgWidth = 5f + maxLineWidth + 5f;
            float bgHeight = 5f + lineHeight + titleGap + lineHeight * scoreDisplayData.size() + 5f;

            Skia.drawBlur(x.get().floatValue(), y.get().floatValue(), bgWidth, bgHeight, 6f);
            Skia.drawShadow(x.get().floatValue(), y.get().floatValue(), bgWidth, bgHeight, 6f);
            Skia.drawRoundedRect(x.get().floatValue(), y.get().floatValue(), bgWidth, bgHeight, 6f, Design.BACKGROUND_COLOR);

            float titleX = x.get().floatValue() + (bgWidth - titleWidth) / 2f;
            float titleY = y.get().floatValue() + 5f;
            Skia.drawText(Skia.getLegacyString(titleComponent), titleX, titleY, Design.TEXT_COLOR, font);

            float startX = x.get().floatValue() + 5f;
            float startY = titleY + lineHeight + titleGap;

            List<Pair<Score, Component>> reverse = Lists.reverse(scoreDisplayData);
            for (Pair<Score, Component> pair : reverse) {
                if (reverse.get(reverse.size() - 1) == pair && replaceIP.get()) {
                    Skia.drawGradientText("www.starain.today", startX, startY, font);
                } else {
                    Skia.drawText(Skia.getLegacyString(pair.getSecond()), startX, startY, Design.TEXT_COLOR, font);
                }
                if (renderScore.get()) {
                    float scoreStartX = x.get().floatValue() + bgWidth - 5f - Skia.getTextWidth(String.valueOf(pair.getFirst().getScore()), font);
                    Skia.drawText("§c" + pair.getFirst().getScore(), scoreStartX, startY, Color.RED, font);
                }
                startY += lineHeight;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}