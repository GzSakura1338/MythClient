package dev.myth.model.render;

import dev.myth.Myth;
import dev.myth.UI.EntityShader;
import dev.myth.UI.setting.Setting;
import dev.myth.model.Category;
import dev.myth.model.Model;
import dev.myth.utils.wrapper.Wrapper;
import java.util.List;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.entity.ambient.AmbientCreature;
import net.minecraft.world.entity.animal.Animal;
import net.minecraft.world.entity.animal.fish.WaterAnimal;
import net.minecraft.world.entity.boss.enderdragon.EndCrystal;
import net.minecraft.world.entity.item.ItemEntity;
import net.minecraft.world.entity.monster.Monster;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.entity.projectile.throwableitemprojectile.ThrownEnderpearl;
import net.minecraft.world.entity.projectile.throwableitemprojectile.ThrownExperienceBottle;
import net.minecraft.world.entity.vehicle.minecart.AbstractMinecart;

/**
 * Mio Shader: an ESP drawn as a post process rather than as lines. Vanilla renders the selected
 * entities into its entity outline target - the same one the glowing effect uses - and one of four
 * fragment shaders then turns that silhouette into fill plus outline.
 *
 * <p>Mio got its silhouette by swapping the world renderer's framebuffer and re-rendering every
 * entity itself. 26.2 builds the world through a frame graph, so that is neither possible nor
 * needed: setting {@code EntityRenderState.outlineColor} is enough. Details and the settings that
 * were left behind are in docs/mio-shader-port.md.
 */
public final class Shader extends Model implements Wrapper {
    private static Shader instance;

    /** Mio multiplies elapsed milliseconds by these; both are the decompile's float constants. */
    private static final float RAINBOW_RATE = .001f, GRADIENT_RATE = .01f;

    public final Setting<String> shader = addSetting(Setting.choice("shader", "Shader", "Solid",
            List.of("Solid", "Rainbow", "Gradient", "Bloom")));
    public final Setting<Integer> range = addSetting(Setting.integer("range", "Range", 60, 1, 300));
    public final Setting<Integer> lineWidth = addSetting(Setting.integer("lineWidth", "LineWidth", 1, 0, 10)
            .description("0 turns the outline off, which is how Mio does it."));
    public final Setting<Boolean> hands = addSetting(Setting.bool("hands", "Hands", true)
            .description("Runs the pass over the first person hand as well. Separate from Self, which is the third person body."));
    public final Setting<Boolean> fastLines = addSetting(Setting.bool("fastLines", "FastLines", false)
            .description("Skips the outline search where a coarse probe finds nothing. Only acts above width 2."));
    public final Setting<Float> glow = addSetting(Setting.decimal("glow", "Glow", 1, 0, 5).step(.05));
    public final Setting<Integer> quality = addSetting(Setting.integer("quality", "Quality", 1, 1, 6)
            .description("Bloom sampling step: larger is faster and coarser.")
            .visibleWhen(() -> shader.get().equals("Bloom")));
    public final Setting<Float> step = addSetting(Setting.decimal("step", "Step", .5f, .1f, 2).step(.05)
            .visibleWhen(() -> shader.get().equals("Gradient")));
    public final Setting<Float> speed = addSetting(Setting.decimal("speed", "Speed", 1, .1f, 5).step(.05)
            .visibleWhen(() -> shader.get().equals("Gradient")));
    public final Setting<Float> fillSpeed = addSetting(Setting.decimal("fillSpeed", "FillSpeed", 1, 0, 5).step(.05)
            .visibleWhen(() -> shader.get().equals("Rainbow")));
    public final Setting<Float> outlineSpeed = addSetting(Setting.decimal("outlineSpeed", "OutlineSpeed", 1, 0, 5).step(.05)
            .visibleWhen(() -> shader.get().equals("Rainbow")));
    public final Setting<Float> fillStrength = addSetting(Setting.decimal("fillStrength", "FillStrength", 1, 0, 5).step(.05)
            .visibleWhen(() -> shader.get().equals("Rainbow")));
    public final Setting<Float> outlineStrength = addSetting(Setting.decimal("outlineStrength", "OutlineStrength", 1, 0, 5).step(.05)
            .visibleWhen(() -> shader.get().equals("Rainbow")));

    public final Setting<Boolean> decorator = addSetting(Setting.bool("decorator", "Decorator", false).group().action());
    public final Setting<String> type = addSetting(Setting.choice("type", "Type", "None", List.of("None", "Dots", "Grid"))
            .visibleWhen(decorator::isExpanded));
    public final Setting<Integer> decoratorRadius = addSetting(Setting.integer("radius", "DecoratorRadius", 5, 1, 20)
            .visibleWhen(decorator::isExpanded));
    public final Setting<Float> decoratorAlpha = addSetting(Setting.decimal("alpha", "DecoratorAlpha", 1, 0, 1).step(.01)
            .visibleWhen(decorator::isExpanded));

    public final Setting<Boolean> colors = addSetting(Setting.bool("colors", "Colors", false).group().action());
    /** Mio has no Fill switch: an alpha of 0 is how the fill is turned off. */
    public final Setting<Integer> fill = addSetting(Setting.color("fill", "Fill", 0x78bd99ff, true, false)
            .visibleWhen(colors::isExpanded));
    public final Setting<Integer> fillSecond = addSetting(Setting.color("fillSecond", "FillSecond", 0x736f599a, true, false)
            .visibleWhen(() -> colors.isExpanded() && shader.get().equals("Gradient")));
    public final Setting<Integer> outline = addSetting(Setting.color("outline", "Outline", 0xffbd99ff, true, false)
            .visibleWhen(colors::isExpanded));
    public final Setting<Integer> outlineSecond = addSetting(Setting.color("outlineSecond", "OutlineSecond", 0xff7c61ae, true, false)
            .visibleWhen(() -> colors.isExpanded() && shader.get().equals("Gradient")));

    public final Setting<Boolean> targets = addSetting(Setting.bool("targets", "Targets", true).group());
    public final Setting<Boolean> players = addSetting(Setting.bool("players", "Players", false).visibleWhen(this::targetsOpen));
    public final Setting<Boolean> self = addSetting(Setting.bool("self", "Self", false)
            .description("Only while the camera is in third person.").visibleWhen(this::targetsOpen));
    public final Setting<Boolean> animals = addSetting(Setting.bool("animals", "Animals", false).visibleWhen(this::targetsOpen));
    public final Setting<Boolean> hostiles = addSetting(Setting.bool("hostiles", "Hostiles", false).visibleWhen(this::targetsOpen));
    public final Setting<Boolean> crystals = addSetting(Setting.bool("crystals", "Crystals", false).visibleWhen(this::targetsOpen));
    public final Setting<Integer> crystalRange = addSetting(Setting.integer("crystalRange", "CrystalRange", 60, 1, 300)
            .visibleWhen(() -> targetsOpen() && crystals.get()));
    public final Setting<Boolean> chests = addSetting(Setting.bool("chests", "Chests", false)
            .description("Chests, trapped chests and ender chests. Not a Mio target; they are block entities.")
            .visibleWhen(this::targetsOpen));
    public final Setting<Integer> chestRange = addSetting(Setting.integer("chestRange", "ChestRange", 60, 1, 300)
            .visibleWhen(() -> targetsOpen() && chests.get()));
    public final Setting<Boolean> items = addSetting(Setting.bool("items", "Items", false).visibleWhen(this::targetsOpen));
    public final Setting<Integer> itemRange = addSetting(Setting.integer("itemRange", "ItemRange", 60, 1, 300)
            .visibleWhen(() -> targetsOpen() && items.get()));
    public final Setting<Boolean> exp = addSetting(Setting.bool("exp", "Exp", false).visibleWhen(this::targetsOpen));
    public final Setting<Boolean> pearls = addSetting(Setting.bool("pearls", "Pearls", false).visibleWhen(this::targetsOpen));
    public final Setting<Boolean> minecarts = addSetting(Setting.bool("minecarts", "Minecarts", false).visibleWhen(this::targetsOpen));
    public final Setting<Boolean> dead = addSetting(Setting.bool("dead", "Dead", false).visibleWhen(this::targetsOpen));

    private final long started = System.currentTimeMillis();
    /** Set by the render state hook, read by the composite: no outlined entity means nothing to draw. */
    private boolean outlinedThisFrame;

    public Shader() {
        super("shader", "Shader", Category.RENDER);
        setDescription("Advanced ESP using shaders.");
        instance = this;
    }

    private boolean targetsOpen() { return targets.get() && targets.isExpanded(); }

    @Override public String getInfo() { return shader.get(); }
    @Override protected void onDisable() { outlinedThisFrame = false; }

    public static Shader active() {
        return instance != null && instance.isEnabled() ? instance : null;
    }

    /** True while the module wants vanilla's own outline post chain suppressed. */
    public static boolean suppressesVanillaOutline() { return active() != null; }

    /**
     * Called from the entity render state hook. Mio picked its entities in a render event and drew
     * them itself; here vanilla does the drawing, so the only decision left is per entity.
     */
    public static boolean shouldOutline(Entity entity) {
        Shader shader = active();
        if (shader == null || !shader.isTarget(entity)) return false;
        shader.outlinedThisFrame = true;
        return true;
    }

    /**
     * Chests are block entities, so they have no EntityRenderState and no outlineColor of their
     * own; what they do have is the outlineColor argument ChestRenderer passes to submitModel,
     * which lands in the same outline group. Mio has no chest target at all - this one is ours.
     */
    public static int chestOutline(net.minecraft.core.BlockPos pos) {
        Shader shader = active();
        if (shader == null || !shader.chests.get() || mc.player == null) return 0;
        if (mc.player.blockPosition().distSqr(pos) > (double) shader.chestRange.get() * shader.chestRange.get()) return 0;
        shader.outlinedThisFrame = true;
        return 0xffffffff;
    }

    public boolean hasOutlines() { return outlinedThisFrame; }
    public void clearFrame() { outlinedThisFrame = false; }

    /** Mio is1763, minus its frustum helper - vanilla already culled anything it hands us. */
    private boolean isTarget(Entity entity) {
        if (mc.player == null || entity == null) return false;
        int limit = entity instanceof EndCrystal ? crystalRange.get()
                : entity instanceof ItemEntity ? itemRange.get() : range.get();
        if (mc.player.distanceTo(entity) > limit) return false;
        if (!entity.isAlive() && !dead.get()) return false;
        if (entity instanceof Player player) {
            if (player.isDeadOrDying() && !dead.get()) return false;
            if (entity == mc.player) return self.get() && !mc.options.getCameraType().isFirstPerson();
            return players.get();
        }
        if (entity instanceof ItemEntity) return items.get();
        if (entity instanceof ThrownExperienceBottle) return exp.get();
        if (entity instanceof ThrownEnderpearl) return pearls.get();
        if (entity instanceof EndCrystal) return crystals.get();
        if (entity instanceof AbstractMinecart) return minecarts.get();
        if (entity instanceof Monster) return hostiles.get();
        if (entity instanceof Animal || entity instanceof AmbientCreature || entity instanceof WaterAnimal) return animals.get();
        return entity instanceof LivingEntity && hostiles.get();
    }

    /** Everything the active fragment shader needs, built once per pass. */
    public EntityShader.Style style() { return style(false); }

    /**
     * @param handsPass Mio forces FastLines on for the hand: it covers a large part of the screen,
     *                  and the full outline search over that area is what makes the mode expensive.
     */
    public EntityShader.Style style(boolean handsPass) {
        float elapsed = System.currentTimeMillis() - started;
        int theme = Myth.UI_MANAGER.getThemeColor();
        return new EntityShader.Style(shader.get(), 2, lineWidth.get(), fastLines.get() || handsPass, glow.get(), quality.get(),
                fill.color(theme), fillSecond.color(theme), outline.color(theme), outlineSecond.color(theme),
                switch (type.get()) { case "Dots" -> 1; case "Grid" -> 2; default -> 0; },
                decoratorRadius.get(), decoratorAlpha.get(),
                elapsed * RAINBOW_RATE * fillSpeed.get(), fillStrength.get(),
                elapsed * RAINBOW_RATE * outlineSpeed.get(), outlineStrength.get(),
                elapsed * GRADIENT_RATE * speed.get(), step.get());
    }
}
