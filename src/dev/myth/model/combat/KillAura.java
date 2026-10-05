package dev.myth.model.combat;

import dev.myth.UI.setting.Setting;
import dev.myth.event.api.EventType;
import dev.myth.event.api.Priority;
import dev.myth.event.api.annotations.EventPriority;
import dev.myth.event.api.annotations.EventTarget;
import dev.myth.event.impl.GameTickEvent;
import dev.myth.event.impl.PlayerTickEvent;
import dev.myth.event.impl.RenderSkiaEvent;
import dev.myth.event.impl.RespawnEvent;
import dev.myth.event.impl.MoveInputEvent;
import dev.myth.model.Category;
import dev.myth.model.Model;
import dev.myth.model.player.Scaffold;
import dev.myth.Myth;
import dev.myth.utils.rotation.RayCastUtil;
import dev.myth.utils.rotation.Rotation;
import dev.myth.utils.rotation.RotationManager;
import dev.myth.utils.rotation.RotationUtils;
import dev.myth.utils.wrapper.Wrapper;
import dev.myth.utils.player.CriticalAttackState;
import dev.myth.utils.player.CriticalSprint;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.ThreadLocalRandom;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import java.util.stream.StreamSupport;
import net.minecraft.util.Mth;
import net.minecraft.sounds.SoundEvents;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.entity.ambient.AmbientCreature;
import net.minecraft.world.entity.animal.Animal;
import net.minecraft.world.entity.animal.equine.AbstractHorse;
import net.minecraft.world.entity.animal.fish.WaterAnimal;
import net.minecraft.world.entity.boss.enderdragon.EnderDragon;
import net.minecraft.world.entity.monster.Monster;
import net.minecraft.world.entity.monster.cubemob.Slime;
import net.minecraft.world.entity.npc.villager.AbstractVillager;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.phys.EntityHitResult;
import net.minecraft.world.phys.HitResult;

/** Starain KillAura; aim and movement correction share the existing RotationManager. */
public final class KillAura extends Model implements Wrapper {
    public static List<Entity> targets;
    public static int targetNumber;
    private final CriticalSprint criticalSprint = new CriticalSprint();
    public final Setting<Boolean> hitSel = addSetting(Setting.bool("hitSelect", "Hit select", true));
    public final Setting<Boolean> attackInvisible = addSetting(Setting.bool("attackInvisible", "Attack Invisible", true));
    public final Setting<String> cpsMode = addSetting(Setting.choice("cpsMode", "CPS Mode", "Default", List.of("Default", "1.9+"))
            .onChange(value -> resetAttackMode()));
    public final Setting<Boolean> waitCritical = addSetting(Setting.bool("waitCritical", "Wait Critical", false).group()
            .description("Waits for airborne criticals; ground, water and cobwebs allow normal attacks.")
            .visibleWhen(this::usesAttackCooldown).onChange(value -> criticalSprint.reset()));
    public final Setting<Boolean> interruptSprint = addSetting(Setting.bool("interruptSprint", "Interrupt Sprint", false)
            .description("Interrupts forward input once during the first block of falling.")
            .visibleWhen(() -> usesAttackCooldown() && waitCritical.isExpanded()).onChange(value -> criticalSprint.reset()));
    public final Setting<Boolean> criticalSound = addSetting(Setting.bool("criticalSound", "Critical Sound", false)
            .visibleWhen(() -> usesAttackCooldown() && waitCritical.isExpanded()));
    public final Setting<Integer> cps = addSetting(Setting.integer("cps", "CPS", 12, 1, 20).visibleWhen(() -> !usesAttackCooldown()));
    public final Setting<Float> aimRange = addSetting(Setting.decimal("aimRange", "Aim range", 4, 3, 6).step(.01));
    public final Setting<Integer> fov = addSetting(Setting.integer("fov", "FOV", 180, 0, 360));
    public final Setting<Integer> rotationSpeed = addSetting(Setting.integer("rotationSpeed", "Rotation speed", 180, 0, 180));
    public final Setting<String> mode = addSetting(Setting.choice("mode", "Mode", "Single", List.of("Single", "Switch")));
    public final Setting<Integer> switchDelay = addSetting(Setting.integer("switchDelay", "Switch delay", 100, 0, 500)
            .unit("ms").visibleWhen(() -> mode.get().equals("Switch")));
    public final Setting<String> priority = addSetting(Setting.choice("priority", "Priority", "Range", List.of("Health", "FoV", "Range", "None")));
    public final Setting<Boolean> targetOptions = addSetting(Setting.bool("targets", "Targets", false).group().action());
    public final Setting<Boolean> players = addSetting(Setting.bool("players", "Player", true).visibleWhen(targetOptions::isExpanded));
    public final Setting<Boolean> invisibles = addSetting(Setting.bool("invisibles", "Invisibles", false).visibleWhen(targetOptions::isExpanded));
    public final Setting<Boolean> animals = addSetting(Setting.bool("animals", "Animals", false).visibleWhen(targetOptions::isExpanded));
    public final Setting<Boolean> mobs = addSetting(Setting.bool("mobs", "Mobs", false).visibleWhen(targetOptions::isExpanded));
    public final Setting<Boolean> villagers = addSetting(Setting.bool("villagers", "Villager", false).visibleWhen(targetOptions::isExpanded));
    // Preserve Starain TimerUtils' initial zero timestamp and wall-clock switch interval.
    private long lastSwitchTime;
    private long lastAttackTime;
    private int attacks;

    public KillAura() {
        super("killaura", "KillAura", Category.COMBAT);
        setDescription("Automatically aims at and attacks selected targets.");
    }

    @EventTarget
    public void onGameTick(GameTickEvent event) {
        if (mc.player == null || mc.level == null) { clearTargets(); return; }
        if (event.getType() != EventType.PRE) return;
        if (scaffoldActive()) { clearTargets(); return; }
        Stream<Entity> stream = StreamSupport.stream(mc.level.entitiesForRendering().spliterator(), true)
                .filter(entity -> isTarget(entity)
                        && (attackInvisible.get() || !(entity instanceof Player && entity.isInvisible()))
                        && RotationUtils.getDistance(entity) <= aimRange.get()
                        // Retain Starain's original signed pitch/yaw FOV comparison.
                        && Mth.wrapDegrees(mc.player.getXRot() - RotationUtils.calculate(entity).getYaw()) <= fov.get());
        targets = stream.collect(Collectors.toList());
        if (priority.get().equals("Range")) {
            targets.sort(Comparator.comparingDouble(entity -> entity.distanceTo(mc.player)));
        } else if (priority.get().equals("FoV")) {
            targets.sort(Comparator.comparingDouble(entity -> Mth.wrapDegrees(mc.player.getXRot() - RotationUtils.calculate(entity).getYaw())));
        } else if (priority.get().equals("Health")) {
            targets.sort(Comparator.comparingDouble(entity -> entity instanceof LivingEntity living ? living.getHealth() : 0));
        }
        switch (mode.get()) {
            case "Single" -> targetNumber = 0;
            case "Switch" -> {
                if (System.currentTimeMillis() - lastSwitchTime >= switchDelay.get()) {
                    lastSwitchTime = System.currentTimeMillis();
                    if (++targetNumber >= targets.size()) targetNumber = 0;
                }
            }
        }
        if (targetNumber >= targets.size()) targetNumber = 0;
        if (targets.isEmpty()) return;

        Entity target = targets.get(targetNumber);
        Rotation calculate = RotationUtils.calculate(target, true, aimRange.get());
        HitResult ray = RayCastUtil.rayCast(calculate, aimRange.get());
        if (ray == null || ray.getType() == HitResult.Type.BLOCK) return;
        RotationManager.setRotations(calculate, rotationSpeed.get(), rotation ->
                RayCastUtil.rayCast(rotation, 3.0f) instanceof EntityHitResult hit && hit.getEntity() == target);

        // Teams/AntiBot/Velocity/Criticals are absent: use their disabled states.
        if (!usesAttackCooldown() && hitSel.get() && mc.hitResult instanceof EntityHitResult hit && hit.getEntity() instanceof Player player) {
            long time = System.currentTimeMillis();
            long delay = attackDelay();
            if (time - lastAttackTime >= delay && (player.hurtTime <= 1 || mc.player.hurtTime >= 6)) {
                attacks++;
                lastAttackTime = time;
            }
        }
    }

    @EventTarget
    public void onPlayerTick(PlayerTickEvent event) {
        if (mc.player == null || mc.level == null || mc.gameMode == null) { clearTargets(); return; }
        if (scaffoldActive()) { clearTargets(); return; }
        if (usesAttackCooldown()) {
            // Discard legacy CPS requests. A charged weapon attacks at most once per callback.
            attacks = 0;
            if (mc.gui.screen() != null || !hasTargets() || mc.player.isUsingItem()) return;
            if (!(mc.hitResult instanceof EntityHitResult hit)) return;
            if (hitSel.get() && hit.getEntity() instanceof Player player
                    && player.hurtTime > 1 && mc.player.hurtTime < 6) return;
            float strength = mc.player.getAttackStrengthScale(0);
            if (!Float.isFinite(strength) || strength < 1) return;
            if (waitCritical.get()) {
                CriticalAttackState state = CriticalAttackState.read(mc.player);
                if (state == null || !state.allowsAttack()) return;
            }
            mc.gameMode.attack(mc.player, hit.getEntity());
            mc.player.swing(InteractionHand.MAIN_HAND);
            lastAttackTime = System.currentTimeMillis();
            return;
        }
        while (attacks > 0) {
            attacks--;
            if (mc.player.isUsingItem()) return;
            if (mc.hitResult instanceof EntityHitResult hit) {
                mc.gameMode.attack(mc.player, hit.getEntity());
                mc.player.swing(InteractionHand.MAIN_HAND);
            }
        }
    }

    @EventTarget
    public void onRender(RenderSkiaEvent event) {
        if (mc.player == null || mc.level == null) { clearTargets(); return; }
        if (scaffoldActive()) { clearTargets(); return; }
        if (usesAttackCooldown()) { attacks = 0; return; }
        if (targets != null && !targets.isEmpty()) {
            if (!hitSel.get() || !(mc.hitResult instanceof EntityHitResult hit && hit.getEntity() instanceof Player)) {
                long time = System.currentTimeMillis();
                if (time - lastAttackTime >= attackDelay()) {
                    attacks++;
                    lastAttackTime = time;
                }
            }
        }
    }

    private long attackDelay() {
        return (long) (1000.0 / Math.max(.01, ThreadLocalRandom.current().nextGaussian(cps.get(), 5)));
    }

    public boolean usesAttackCooldown() { return cpsMode.get().equals("1.9+"); }
    private boolean hasTargets() { return targets != null && !targets.isEmpty(); }
    private boolean scaffoldActive() { return Myth.MODEL_MANAGER.getModel(Scaffold.class).map(Model::isEnabled).orElse(false); }
    private boolean criticalSprintEnabled() {
        return isEnabled() && mc.player != null && mc.level != null && mc.gui.screen() == null
                && !scaffoldActive() && usesAttackCooldown() && waitCritical.get() && interruptSprint.get();
    }

    @EventTarget @EventPriority(Priority.HIGH)
    public void onCriticalSprintTick(GameTickEvent event) {
        if (event.getType() != EventType.PRE) return;
        criticalSprint.reset();
        if (!criticalSprintEnabled() || !hasTargets()) return;
        CriticalAttackState state = CriticalAttackState.read(mc.player);
        if (state == null || state.ground() || state.water() || state.web()) return;
        criticalSprint.tick(state.fallDistance(), true, 0);
    }

    @EventTarget
    public void onCriticalSprintInput(MoveInputEvent event) {
        if (!criticalSprintEnabled() || !hasTargets()) { criticalSprint.reset(); return; }
        // Vanilla aiStep owns sprint recovery and packets; only consume forward input.
        criticalSprint.input(event);
    }

    /** Called from the local player's actual vanilla crit callback, never from a swing. */
    public void onCriticalHit() {
        if (isEnabled() && usesAttackCooldown() && waitCritical.get() && criticalSound.get() && mc.player != null)
            mc.player.playSound(SoundEvents.EXPERIENCE_ORB_PICKUP, 1, .9f);
    }

    private void resetAttackMode() { attacks = 0; criticalSprint.reset(); }

    private boolean isTarget(Entity entity) {
        if (!(entity instanceof LivingEntity) || entity == mc.player || !entity.isAlive()) return false;
        if (entity instanceof Player && entity.isSpectator()) return false;
        if (entity.isInvisible() && !invisibles.get()) return false;
        if (entity instanceof Player) return players.get();
        if (entity instanceof AbstractVillager) return villagers.get();
        if (entity instanceof Monster || entity instanceof Slime || entity instanceof EnderDragon) return mobs.get();
        if (entity instanceof Animal || entity instanceof AmbientCreature || entity instanceof WaterAnimal || entity instanceof AbstractHorse) return animals.get();
        return false;
    }

    @Override public void onDisable() { clearTargets(); }
    @EventTarget public void onRespawn(RespawnEvent event) { clearTargets(); }
    private void clearTargets() { targets = null; attacks = 0; targetNumber = 0; criticalSprint.reset(); }
    public boolean isSwitchMode() { return mode.get().equals("Switch"); }
    public static Entity getTarget() {
        if (targets == null || targets.isEmpty() || targetNumber < 0 || targetNumber >= targets.size()) return null;
        return targets.get(targetNumber);
    }
}
