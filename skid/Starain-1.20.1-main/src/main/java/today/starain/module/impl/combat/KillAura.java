package today.starain.module.impl.combat;

import net.minecraft.util.Mth;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.phys.EntityHitResult;
import net.minecraft.world.phys.HitResult;
import today.starain.Starain;
import today.starain.event.api.EventType;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.GameTickEvent;
import today.starain.event.impl.PlayerTickEvent;
import today.starain.event.impl.RenderSkiaEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.module.impl.misc.AntiBot;
import today.starain.module.impl.misc.Target;
import today.starain.module.impl.misc.Teams;
import today.starain.module.impl.player.Scaffold;
import today.starain.utils.rotation.RayCastUtil;
import today.starain.utils.rotation.Rotation;
import today.starain.utils.rotation.RotationManager;
import today.starain.utils.rotation.RotationUtils;
import today.starain.utils.timer.TimerUtils;
import today.starain.value.impl.BooleanValue;
import today.starain.value.impl.ModeValue;
import today.starain.value.impl.NumberValue;

import java.util.Comparator;
import java.util.List;
import java.util.concurrent.ThreadLocalRandom;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import java.util.stream.StreamSupport;

public class KillAura extends ClientModule {

    public static List<Entity> targets;
    private final BooleanValue hitSel = new BooleanValue("Hit select", true, this);
    private final BooleanValue attackInvisible = new BooleanValue("Attack Invisible", true, this);
    private final NumberValue cps = new NumberValue("CPS", 1.0, 20.0, 12.0, 1.0, this);
    private final NumberValue aimRange = new NumberValue("Aim range", 3.0, 6.0, 4.0, 0.01, this);
    private final NumberValue fov = new NumberValue("FOV", 0.0, 360.0, 180.0, 1.0, this);
    private final NumberValue rotationSpeed = new NumberValue("Rotation speed", 0.0, 180.0, 180.0, 1.0, this);
    private final ModeValue mode = new ModeValue("Mode", "Single", new String[]{"Single", "Switch"}, this);
    private final NumberValue switchDelay = new NumberValue("Switch delay", 0.0, 500.0, 100.0, 1.0, this, () -> mode.isMode("Switch"));
    private final ModeValue priority = new ModeValue("Priority", "Range", new String[]{"Health", "FoV", "Range", "None"}, this);
    private final TimerUtils switchTimer = new TimerUtils();
    public static int targetNumber;
    private long lastAttackTime;
    private int attacks;

    public KillAura() {
        setName("KillAura");
        setCategory(Category.COMBAT);
    }

    @EventTarget
    public void onGameTick(GameTickEvent event) {
        if (mc.player == null || event.getType() != EventType.PRE || Starain.getInstance().getModuleManager().getModule(Scaffold.class).isEnabled())
            return;
        Stream<Entity> stream = StreamSupport.stream(mc.level.entitiesForRendering().spliterator(), true)
                .filter(entity -> entity instanceof Entity)
                .filter((entity) -> {
                    boolean b = Target.isTarget(entity)
                            && (attackInvisible.get() || !(entity instanceof Player && entity.isInvisible()))
                            && !Teams.isSameTeam(entity)
                            && !AntiBot.isBot(entity)
                            && RotationUtils.getDistance(entity) <= aimRange.get().floatValue()
                            && Mth.wrapDegrees(mc.player.getXRot() - RotationUtils.calculate(entity).getYaw()) <= fov.get().intValue();
                    return b;
                });
        targets = stream.collect(Collectors.toList());
        if (this.priority.isMode("Range")) {
            targets.sort(Comparator.comparingDouble(o -> (double) o.distanceTo(mc.player)));
        } else if (this.priority.isMode("FoV")) {
            targets.sort(
                    Comparator.comparingDouble(o -> (double) Mth.wrapDegrees(mc.player.getXRot() - RotationUtils.calculate(o).getYaw()))
            );
        } else if (this.priority.isMode("Health")) {
            targets.sort(Comparator.comparingDouble(o -> o instanceof LivingEntity living ? (double) living.getHealth() : 0.0));
        }

        if (Velocity.delay) {
            targets.sort(
                    Comparator.comparingDouble(o -> (double) Math.abs(Velocity.yaw - RotationUtils.calculate(o).getYaw()))
            );
        }

        switch (mode.get()) {
            case "Single" -> {
                targetNumber = 0;
            }
            case "Switch" -> {
                if (switchTimer.delay(switchDelay.get().intValue())) {
                    switchTimer.reset();
                    if (++targetNumber >= targets.size()) {
                        targetNumber = 0;
                    }
                }
            }
        }

        if (targetNumber >= targets.size()) {
            targetNumber = 0;
        }

        if (targets == null || targets.isEmpty()) return;

        Entity target = targets.get(targetNumber);
        Rotation calculate = RotationUtils.calculate(target, true, aimRange.get().floatValue());
        if (RayCastUtil.rayCast(calculate, aimRange.get().floatValue()).getType() == HitResult.Type.BLOCK) return;
        RotationManager.setRotations(calculate, rotationSpeed.get().intValue(), rotation -> RayCastUtil.rayCast(rotation, 3.0f) instanceof EntityHitResult entityHitResult && entityHitResult.getEntity() == target);

        if (hitSel.get() && mc.hitResult instanceof EntityHitResult entityHitResult && entityHitResult.getEntity() instanceof Player player && !AntiBot.isBot(player) && !Teams.isSameTeam(player) && Velocity.attackQueue <= 0) {
            long time = System.currentTimeMillis();
            long delay = (long) (1000.0 / (Math.max(0.01, ThreadLocalRandom.current().nextGaussian(cps.get().floatValue(), 5))));
            if (time - lastAttackTime >= delay) {
                if (player.hurtTime <= 1 || Criticals.fallTicks == 2 || (mc.player.hurtTime >= 6 && !Starain.getInstance().getModuleManager().getModule(Velocity.class).isEnabled())) {
                    attacks++;
                    lastAttackTime = time;
                }
            }
        }
    }

    @EventTarget
    public void onPlayerTick(PlayerTickEvent event) {

        while (attacks > 0) {
            attacks--;
            if (mc.player.isUsingItem()) return;
            if (mc.hitResult instanceof EntityHitResult entityHitResult) {
                mc.gameMode.attack(mc.player, entityHitResult.getEntity());
                mc.player.swing(InteractionHand.MAIN_HAND);
            }
        }
    }

    @EventTarget
    public void onRender(RenderSkiaEvent event) {

        if (targets != null && !targets.isEmpty() && Velocity.attackQueue <= 0) {
            if (!hitSel.get() || !(mc.hitResult instanceof EntityHitResult entityHitResult && entityHitResult.getEntity() instanceof Player)) {
                long time = System.currentTimeMillis();
                long delay = (long) (1000.0 / (Math.max(0.01, ThreadLocalRandom.current().nextGaussian(cps.get().floatValue(), 5))));
                if (time - lastAttackTime >= delay) {
                    attacks++;
                    lastAttackTime = time;
                }
            }
        }
    }

    @Override
    public void onDisable() {
        targets = null;
        attacks = 0;
    }

    public boolean isSwitchMode() {
        return mode.isMode("Switch");
    }

    @Override
    public String getSuffix() {
        return mode.get();
    }

    public static Entity getTarget() {
        if (targets == null || targets.isEmpty()) return null;
        return targets.get(targetNumber);
    }
}
