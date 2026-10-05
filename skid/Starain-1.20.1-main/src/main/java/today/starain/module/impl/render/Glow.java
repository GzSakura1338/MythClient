package today.starain.module.impl.render;

import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.item.ItemEntity;
import net.minecraft.world.level.block.entity.BarrelBlockEntity;
import net.minecraft.world.level.block.entity.BlockEntity;
import net.minecraft.world.level.block.entity.ChestBlockEntity;
import net.minecraft.world.level.block.entity.EnderChestBlockEntity;
import net.minecraft.world.level.block.entity.ShulkerBoxBlockEntity;
import net.minecraft.world.level.chunk.ChunkStatus;
import net.minecraft.world.level.chunk.LevelChunk;
import net.minecraft.world.entity.player.Player;
import today.starain.Starain;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.RenderWorldEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.module.impl.misc.Target;
import today.starain.ui.Theme;
import today.starain.utils.render.shader.impl.OutlineGlow;
import today.starain.value.impl.BooleanValue;
import today.starain.value.impl.ColorValue;
import today.starain.value.impl.ModeValue;
import today.starain.value.impl.NumberValue;

import java.awt.Color;
import java.util.ArrayList;
import java.util.List;

public class Glow extends ClientModule {

    private static final int MAX_STOPS = 128;


    private final BooleanValue syncColor = new BooleanValue("Sync Color", true, this);
    private final ColorValue color = new ColorValue("Color", new Color(0, 186, 255), this, () -> !syncColor.get());
    private final NumberValue radius = new NumberValue("Radius", 1.0, 32.0, 7.0, 1.0, this);
    private final NumberValue strength = new NumberValue("Strength", 0.1, 3.0, 1.0, 0.1, this);
    private final NumberValue spread = new NumberValue("Color Spread", 1.0, 200.0, 50.0, 1.0, this, syncColor::get);
    private final ModeValue blend = new ModeValue("Blend", "Normal", new String[]{"Normal", "Additive"}, this);
    private final BooleanValue innerGlow = new BooleanValue("Inner Glow", true, this);
    private final BooleanValue antiAliasing = new BooleanValue("Anti-Aliasing", true, this);
    private final BooleanValue useTarget = new BooleanValue("Use Target", false, this);
    private final BooleanValue players = new BooleanValue("Players", true, this, () -> !useTarget.get());
    private final BooleanValue items = new BooleanValue("Items", false, this);
    private final BooleanValue chests = new BooleanValue("Chests", false, this);
    private final BooleanValue self = new BooleanValue("Self", false, this);

    public Glow() {
        setName("Glow");
        setCategory(Category.RENDER);
    }

    @EventTarget
    public void onRenderWorld(RenderWorldEvent event) {
        if (mc.level == null || mc.player == null) return;

        boolean firstPerson = mc.options.getCameraType().isFirstPerson();
        List<Entity> targets = new ArrayList<>();
        if (useTarget.get()) {
            for (Entity entity : mc.level.entitiesForRendering()) {
                if (Target.isTarget(entity)) {
                    targets.add(entity);
                }
            }
            if (self.get() && !firstPerson && mc.player.isAlive()) {
                targets.add(mc.player);
            }
        } else if (players.get()) {
            for (Player player : mc.level.players()) {
                if (player == mc.player && (!self.get() || firstPerson)) continue;
                if (!player.isAlive() || player.isSpectator() || player.isInvisible()) continue;
                targets.add(player);
            }
        }
        if (items.get()) {
            for (Entity entity : mc.level.entitiesForRendering()) {
                if (entity instanceof ItemEntity item && item.isAlive()) {
                    targets.add(item);
                }
            }
        }
        List<BlockEntity> blockTargets = new ArrayList<>();
        if (chests.get()) {
            int pcx = mc.player.blockPosition().getX() >> 4;
            int pcz = mc.player.blockPosition().getZ() >> 4;
            int r = 8;
            for (int cx = pcx - r; cx <= pcx + r; cx++) {
                for (int cz = pcz - r; cz <= pcz + r; cz++) {
                    LevelChunk chunk = mc.level.getChunkSource().getChunk(cx, cz, ChunkStatus.FULL, false);
                    if (chunk == null) continue;
                    for (BlockEntity be : chunk.getBlockEntities().values()) {
                        if (be instanceof ChestBlockEntity
                                || be instanceof EnderChestBlockEntity
                                || be instanceof ShulkerBoxBlockEntity
                                || be instanceof BarrelBlockEntity) {
                            blockTargets.add(be);
                        }
                    }
                }
            }
        }

        boolean handGlow = self.get() && firstPerson && (useTarget.get() || players.get());
        if (targets.isEmpty() && blockTargets.isEmpty() && !handGlow) return;

        Palette palette = buildPalette();

        OutlineGlow.INSTANCE.draw(event.getPoseStack(), event.getPartialTick(), targets, blockTargets,
                palette.colors(), palette.stops(), palette.alpha(),
                radius.get().floatValue(), strength.get().floatValue(),
                antiAliasing.get(), handGlow, blend.isMode("Additive"), innerGlow.get());
    }

    private static final List<float[]> hudRects = new ArrayList<>();
    private static final List<float[]> hudRectsHorizontal = new ArrayList<>();

    public static void queueHud(float x, float y, float width, float height, float radius) {
        queueHud(x, y, width, height, radius, 1.0F);
    }

    public static void queueHud(float x, float y, float width, float height, float radius, float alpha) {
        queueHud(x, y, width, height, radius, alpha, false);
    }

    public static void queueHud(float x, float y, float width, float height, float radius, float alpha, boolean horizontal) {
        if (alpha <= 0.004F) {
            return;
        }
        float[] rect = {x, y, width, height, radius, Math.min(alpha, 1.0F)};
        if (horizontal) {
            hudRectsHorizontal.add(rect);
        } else {
            hudRects.add(rect);
        }
    }

    public static void flushHud() {
        if (!hudRects.isEmpty()) {
            drawPanels(new ArrayList<>(hudRects), false);
            hudRects.clear();
        }
        if (!hudRectsHorizontal.isEmpty()) {
            drawPanels(new ArrayList<>(hudRectsHorizontal), true);
            hudRectsHorizontal.clear();
        }
    }

    public static void drawPanels(List<float[]> rects) {
        drawPanels(rects, false);
    }

    public static void drawPanels(List<float[]> rects, boolean horizontal) {
        Glow module = get();
        if (module == null || rects.isEmpty()) {
            return;
        }
        Palette palette = module.buildHudPalette(horizontal);
        OutlineGlow.INSTANCE.drawRects(rects, palette.colors(), palette.stops(), palette.alpha(),
                module.radius.get().floatValue(), module.strength.get().floatValue(),
                module.antiAliasing.get(), module.blend.isMode("Additive"), module.innerGlow.get(), !horizontal);
    }

    private Palette buildHudPalette(boolean horizontal) {
        if (!syncColor.get()) {
            return buildPalette();
        }
        float rate = 0.1F;
        int span = horizontal ? mc.getWindow().getGuiScaledWidth() : mc.getWindow().getGuiScaledHeight();
        int stops = Math.max(2, Math.min(MAX_STOPS, Math.round(span * rate) + 1));
        float[] colors = new float[stops * 3];
        for (int i = 0; i < stops; i++) {
            float pixel = span * i / (float) (stops - 1);
            Color stop = Theme.accent((int) (-pixel * rate));
            colors[i * 3] = stop.getRed() / 255.0F;
            colors[i * 3 + 1] = stop.getGreen() / 255.0F;
            colors[i * 3 + 2] = stop.getBlue() / 255.0F;
        }
        return new Palette(colors, stops, 1.0F);
    }

    private Palette buildPalette() {
        int stops;
        float alpha;
        float rate = spread.get().floatValue() / 100.0F;
        int guiWidth = mc.getWindow().getGuiScaledWidth();
        if (syncColor.get()) {
            stops = Math.max(2, Math.min(MAX_STOPS, Math.round(guiWidth * rate) + 1));
            alpha = 1.0F;
        } else {
            stops = 2;
            alpha = color.get().getAlpha() / 255.0F;
        }

        float[] colors = new float[stops * 3];
        for (int i = 0; i < stops; i++) {
            Color stop;
            if (syncColor.get()) {
                float pixel = guiWidth * i / (float) (stops - 1);
                stop = Theme.accent((int) (-pixel * rate));
            } else {
                stop = color.get();
            }
            colors[i * 3] = stop.getRed() / 255.0F;
            colors[i * 3 + 1] = stop.getGreen() / 255.0F;
            colors[i * 3 + 2] = stop.getBlue() / 255.0F;
        }
        return new Palette(colors, stops, alpha);
    }

    private static Glow get() {
        Starain starain = Starain.getInstance();
        if (starain == null || starain.getModuleManager() == null) {
            return null;
        }
        return starain.getModuleManager().getModule(Glow.class);
    }

    private record Palette(float[] colors, int stops, float alpha) {
    }
}
