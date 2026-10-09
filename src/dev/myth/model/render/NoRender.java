package dev.myth.model.render;

import dev.myth.Myth;
import dev.myth.UI.setting.Selection;
import dev.myth.UI.setting.Setting;
import dev.myth.model.Category;
import dev.myth.model.Model;
import java.util.List;
import java.util.Set;
import net.minecraft.core.particles.ParticleOptions;
import net.minecraft.core.particles.ParticleType;
import net.minecraft.core.particles.ParticleTypes;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.EntityType;

/**
 * Mio NoRender. Setting order, names, defaults and ranges come from Mio's own
 * {@code nick/Setting*} tables, so the four sections stay collapsed exactly like there.
 * The section toggles themselves carry no behaviour in Mio either.
 */
public final class NoRender extends Model {
    /** Mio's floor for a non-zero opacity; below it a model is invisible but still translucent. */
    private static final float MIN_OPACITY = .11f;

    public final Setting<Boolean> self = addSetting(Setting.bool("self", "Self", false).group());
    public final Setting<Boolean> hurtCam = addSetting(Setting.bool("hurtCam", "HurtCam", true)
            .visibleWhen(self::isExpanded));
    public final Setting<Boolean> potions = addSetting(Setting.bool("potions", "Potions", false)
            .visibleWhen(self::isExpanded));
    public final Setting<Boolean> darkness = addSetting(Setting.bool("darkness", "Darkness", true)
            .visibleWhen(self::isExpanded));
    public final Setting<Boolean> blindness = addSetting(Setting.bool("blindness", "Blindness", true)
            .visibleWhen(self::isExpanded));
    public final Setting<Boolean> newSneaking = addSetting(Setting.bool("newSneaking", "NewSneaking", false)
            .visibleWhen(self::isExpanded));
    public final Setting<Boolean> totemOverlay = addSetting(Setting.bool("totemOverlay", "TotemOverlay", false)
            .visibleWhen(self::isExpanded));
    public final Setting<Boolean> hands = addSetting(Setting.bool("hands", "Hands", false).group()
            .visibleWhen(self::isExpanded));
    public final Setting<Float> handsOpacity = addSetting(Setting.decimal("handsOpacity", "Opacity", 0, 0, 1)
            .step(.01).visibleWhen(() -> self.isExpanded() && hands.isExpanded()));

    public final Setting<Boolean> ui = addSetting(Setting.bool("ui", "UI", false).group());
    public final Setting<Boolean> messageIndicator = addSetting(Setting.bool("messageIndicator", "MessageIndicator", true)
            .visibleWhen(ui::isExpanded));
    public final Setting<Boolean> unsecureServer = addSetting(Setting.bool("unsecureServer", "UnsecureServer", true)
            .visibleWhen(ui::isExpanded));
    public final Setting<Boolean> tutorialToast = addSetting(Setting.bool("tutorialToast", "TutorialToast", true)
            .visibleWhen(ui::isExpanded));
    public final Setting<Boolean> textShadow = addSetting(Setting.bool("textShadow", "TextShadow", false)
            .visibleWhen(ui::isExpanded));
    public final Setting<Boolean> bossBars = addSetting(Setting.bool("bossBars", "BossBars", false)
            .visibleWhen(ui::isExpanded));
    public final Setting<Boolean> scoreBoard = addSetting(Setting.bool("scoreBoard", "ScoreBoard", false)
            .visibleWhen(ui::isExpanded));
    public final Setting<Boolean> tabIcons = addSetting(Setting.bool("tabIcons", "TabIcons", false)
            .visibleWhen(ui::isExpanded));
    public final Setting<Boolean> heldTooltips = addSetting(Setting.bool("heldTooltips", "HeldTooltips", false)
            .visibleWhen(ui::isExpanded));
    public final Setting<Boolean> tint = addSetting(Setting.bool("tint", "Tint", false)
            .visibleWhen(ui::isExpanded));

    public final Setting<Boolean> world = addSetting(Setting.bool("world", "World", false).group());
    public final Setting<Boolean> explosions = addSetting(Setting.bool("explosions", "Explosions", true)
            .visibleWhen(world::isExpanded));
    public final Setting<Boolean> worldBorder = addSetting(Setting.bool("worldBorder", "WorldBorder", true)
            .visibleWhen(world::isExpanded));
    public final Setting<Boolean> skyLight = addSetting(Setting.bool("skyLight", "SkyLight", false)
            .visibleWhen(world::isExpanded));
    public final Setting<Boolean> fog = addSetting(Setting.bool("fog", "Fog", false).group()
            .visibleWhen(world::isExpanded));
    public final Setting<Float> fogRange = addSetting(Setting.decimal("range", "Range", 3, .1f, 10)
            .step(.1).visibleWhen(() -> world.isExpanded() && fog.isExpanded()));
    public final Setting<Boolean> fogSky = addSetting(Setting.bool("sky", "Sky", false)
            .visibleWhen(() -> world.isExpanded() && fog.isExpanded()));
    public final Setting<Boolean> particles = addSetting(Setting.bool("particles", "Particles", false).group()
            .visibleWhen(world::isExpanded));
    public final Setting<List<String>> particleTypes = addSetting(
            Selection.registry("particleTypes", "ParticleTypes", BuiltInRegistries.PARTICLE_TYPE)
                    .visibleWhen(() -> world.isExpanded() && particles.isExpanded()));
    public final Setting<String> particleSelection = addSetting(
            Setting.choice("particleSelection", "Selection", "BlackList", Selection.MODES)
                    .visibleWhen(() -> world.isExpanded() && particles.isExpanded()));
    public final Setting<Boolean> noBlockBreaking = addSetting(Setting.bool("noBlockBreaking", "NoBlockBreaking", false)
            .visibleWhen(() -> world.isExpanded() && particles.isExpanded()));

    public final Setting<Boolean> entities = addSetting(Setting.bool("entitiesPage", "Entities", false).group());
    public final Setting<Boolean> hurt = addSetting(Setting.bool("hurt", "Hurt", true)
            .visibleWhen(entities::isExpanded));
    public final Setting<Boolean> nameTags = addSetting(Setting.bool("nameTags", "NameTags", false)
            .visibleWhen(entities::isExpanded));
    public final Setting<Boolean> interpolation = addSetting(Setting.bool("interpolation", "Interpolation", false)
            .visibleWhen(entities::isExpanded));
    public final Setting<Boolean> fire = addSetting(Setting.bool("fire", "Fire", true).group()
            .visibleWhen(entities::isExpanded));
    public final Setting<Boolean> fireOthers = addSetting(Setting.bool("others", "Others", false)
            .visibleWhen(() -> entities.isExpanded() && fire.isExpanded()));
    public final Setting<Boolean> armor = addSetting(Setting.bool("armor", "Armor", false).group()
            .visibleWhen(entities::isExpanded));
    public final Setting<Float> armorOpacity = addSetting(Setting.decimal("armorOpacity", "Opacity", 0, 0, 1)
            .step(.01).visibleWhen(() -> entities.isExpanded() && armor.isExpanded()));
    public final Setting<Boolean> tileEntities = addSetting(Setting.bool("tileEntities", "TileEntities", false).group()
            .visibleWhen(entities::isExpanded));
    public final Setting<Integer> tileDistance = addSetting(Setting.integer("tileDistance", "TileDistance", 64, 1, 256)
            .unit("m").visibleWhen(() -> entities.isExpanded() && tileEntities.isExpanded()));
    public final Setting<Boolean> wardens = addSetting(Setting.bool("wardens", "Wardens", false).group()
            .visibleWhen(entities::isExpanded));
    public final Setting<Integer> wardenDistance = addSetting(Setting.integer("wardenDistance", "WardenDistance", 32, 1, 128)
            .unit("m").visibleWhen(() -> entities.isExpanded() && wardens.isExpanded()));
    public final Setting<Boolean> entityFilter = addSetting(Setting.bool("entities", "Entities", false).group()
            .visibleWhen(entities::isExpanded));
    public final Setting<List<String>> entityTypes = addSetting(
            Selection.registry("entityTypes", "EntityTypes", BuiltInRegistries.ENTITY_TYPE)
                    .visibleWhen(() -> entities.isExpanded() && entityFilter.isExpanded()));
    public final Setting<String> removal = addSetting(Setting.choice("removal", "Removal", "Partial", List.of("Partial", "Full"))
            .visibleWhen(() -> entities.isExpanded() && entityFilter.isExpanded()));
    public final Setting<String> entitySelection = addSetting(
            Setting.choice("entitySelection", "Selection", "BlackList", Selection.MODES)
                    .visibleWhen(() -> entities.isExpanded() && entityFilter.isExpanded()));

    public NoRender() {
        super("norender", "NoRender", Category.RENDER);
        setDescription("Removes the annoying stuff from your screen.");
    }

    /** Mio keeps this module out of its ArrayList. */
    @Override public boolean isDrawn() { return false; }

    /** Mio looks the module up straight out of its manager inside every hook. */
    public static NoRender active() {
        NoRender model = Myth.MODEL_MANAGER.getModel(NoRender.class).orElse(null);
        return model != null && model.isEnabled() ? model : null;
    }

    /** Mio is179: Hands only skips the whole pass once Opacity reaches zero. */
    public boolean hidesHands() { return hands.get() && handsOpacity.get() == 0; }
    /** Mio get1995 / get1996, including the shared floor. */
    public float handOpacity() { return opacity(hands.get(), handsOpacity.get()); }
    public float armorOpacity() { return opacity(armor.get(), armorOpacity.get()); }
    private static float opacity(boolean group, float value) {
        if (!group) return 1;
        return value == 0 ? 0 : Math.max(value, MIN_OPACITY);
    }

    /** Mio's SpellParticle covers the whole potion family. */
    private static final Set<ParticleType<?>> SPELL = Set.of(ParticleTypes.EFFECT, ParticleTypes.INSTANT_EFFECT,
            ParticleTypes.ENTITY_EFFECT, ParticleTypes.WITCH);

    /** Mio onAddParticle (Potions / Explosions) and is1991 (the type set), in one decision. */
    public static boolean hidesParticleType(ParticleOptions particle) {
        NoRender noRender = active();
        if (noRender == null) return false;
        ParticleType<?> type = particle.getType();
        if (noRender.potions.get() && SPELL.contains(type)) return true;
        if (noRender.explosions.get() && (type == ParticleTypes.EXPLOSION || type == ParticleTypes.EXPLOSION_EMITTER)) return true;
        return noRender.hidesParticle(particle);
    }

    /** Mio is1991: true removes the particle. */
    public boolean hidesParticle(ParticleOptions particle) {
        if (!particles.get()) return false;
        String id = BuiltInRegistries.PARTICLE_TYPE.getKey(particle.getType()).toString();
        return !Selection.allows(particleSelection.get(), id, particleTypes.get());
    }

    /** Mio is1992 / is1993: true removes the entity. */
    public boolean hidesEntity(Entity entity) { return hidesEntity(entity.getType()); }
    public boolean hidesEntity(EntityType<?> type) {
        if (!entityFilter.get()) return false;
        String id = BuiltInRegistries.ENTITY_TYPE.getKey(type).toString();
        return !Selection.allows(entitySelection.get(), id, entityTypes.get());
    }

    /** Mio is1994. */
    public boolean hidesBreakingParticles() { return particles.get() && noBlockBreaking.get(); }
    /** Mio removal == FULL also drops the spawn packet, so the entity never reaches the world. */
    public boolean dropsEntitySpawn() { return removal.get().equals("Full"); }
}
