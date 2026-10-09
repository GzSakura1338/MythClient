package dev.myth.model.combat;

import dev.myth.Myth;
import dev.myth.UI.setting.Setting;
import dev.myth.event.api.EventType;
import dev.myth.event.api.annotations.EventTarget;
import dev.myth.event.impl.GameTickEvent;
import dev.myth.event.impl.MotionEvent;
import dev.myth.event.impl.PacketEvent;
import dev.myth.event.impl.RespawnEvent;
import dev.myth.event.impl.StuckInBlockEvent;
import dev.myth.model.Category;
import dev.myth.model.Model;
import dev.myth.utils.wrapper.Wrapper;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.ThreadLocalRandom;
import net.minecraft.client.multiplayer.ClientPacketListener;
import net.minecraft.network.protocol.Packet;
import net.minecraft.network.protocol.common.ClientboundKeepAlivePacket;
import net.minecraft.network.protocol.game.ClientboundDamageEventPacket;
import net.minecraft.network.protocol.game.ClientboundExplodePacket;
import net.minecraft.network.protocol.game.ClientboundLoginPacket;
import net.minecraft.network.protocol.game.ClientboundRespawnPacket;
import net.minecraft.network.protocol.game.ClientboundSetEntityMotionPacket;
import net.minecraft.network.protocol.game.ServerboundAttackPacket;
import net.minecraft.server.RunningOnDifferentThreadException;
import net.minecraft.util.Mth;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.entity.decoration.ArmorStand;
import net.minecraft.world.entity.player.Input;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.Vec3;

/**
 * OpenOnyx VelocityModule, ported from its 1.8.9 decompile. The single Lag mode holds the
 * knockback packet - and every packet queued behind it - for a few ticks and then replays the
 * backlog, while Reduce knockback counter-hits the player in the crosshair to cut the knockback
 * short. Field names from the decompile are kept in comments; it had been through a renamer.
 */
public final class Velocity extends Model implements Wrapper {
    /** OpenOnyx DOUBLE: the radius the target scan uses, not a setting over there either. */
    private static final double TARGET_RANGE = 10;
    /** OpenOnyx DOUBLE2: how far the counter-hit ray reaches. */
    private static final double REDUCE_REACH = 3;

    public final Setting<String> mode = addSetting(Setting.choice("mode", "Mode", "Lag", List.of("Lag")));
    public final Setting<Boolean> delayKnockback = addSetting(Setting.bool("delayKnockback", "Delay knockback", true).group()
            .description("Holds the knockback after a melee hit so it lands a moment late"));
    public final Setting<Integer> delayTicks = addSetting(Setting.integer("delayTicks", "Delay ticks", 3, 1, 20)
            .unit("t").description("How long the knockback is held before it is applied")
            .visibleWhen(() -> delayKnockback.get() && delayKnockback.isExpanded()));
    public final Setting<Boolean> releaseOnGround = addSetting(Setting.bool("releaseOnGround", "Release on ground", true)
            .description("Stops holding the moment you touch the ground"));
    public final Setting<Boolean> releaseAfterReduce = addSetting(Setting.bool("releaseAfterReduce", "Release after reduce", false)
            .description("Stops holding once a counter-hit has reduced the knockback"));
    public final Setting<Boolean> reduceKnockback = addSetting(Setting.bool("reduceKnockback", "Reduce knockback", true).group()
            .description("Counter-hits the attacker in your crosshair to cut the knockback you take"));
    public final Setting<Integer> reduceWindow = addSetting(Setting.integer("reduceWindow", "Reduce window", 5, 0, 20)
            .unit("t").description("How long after knockback the counter-hit may still fire")
            .visibleWhen(() -> reduceKnockback.get() && reduceKnockback.isExpanded()));
    public final Setting<Integer> chance = addSetting(Setting.integer("chance", "Chance", 100, 0, 100)
            .unit("%").description("Chance to reduce each incoming knockback"));
    public final Setting<Integer> fov = addSetting(Setting.integer("fov", "FOV", 360, 0, 360).unit("°"));
    public final Setting<Boolean> pauseBackpedaling = addSetting(Setting.bool("pauseBackpedaling", "Pause while backpedaling", false));
    public final Setting<Boolean> onlyWhileMoving = addSetting(Setting.bool("onlyWhileMoving", "Only while moving", false));
    public final Setting<Boolean> targets = addSetting(Setting.bool("targets", "Targets", false).group().action());
    public final Setting<Boolean> targetPlayers = addSetting(Setting.bool("targetPlayers", "Players", true)
            .visibleWhen(targets::isExpanded));
    public final Setting<Boolean> targetTeammates = addSetting(Setting.bool("targetTeammates", "Teammates", true)
            .visibleWhen(targets::isExpanded));

    /** list2. Written from the netty thread, drained from the client thread. */
    private final List<Packet<?>> held = new CopyOnWriteArrayList<>();
    private volatile boolean holding;    // bool8
    private volatile boolean meleeHit;   // bool4
    private volatile boolean explosion;  // bool5
    private volatile boolean targetNear; // bool7
    private int heldTicks;               // int_
    private int heldLimit;               // int_2
    private boolean reduced;             // bool2
    private boolean windowOpen;          // bool6
    private boolean reduceArmed;         // bool3
    private int windowTicks;             // int_3
    private boolean stuck;               // 1.8 isInWeb, refreshed per tick

    public Velocity() {
        super("velocity", "Velocity", Category.COMBAT);
        setDescription("Delays and counter-hit reduces the velocity received");
    }

    @Override public String getInfo() { return mode.get(); }

    /** OpenOnyx run80. */
    @Override protected void onEnable() { heldLimit = delayTicks.get(); }

    /** OpenOnyx run79: let the backlog go, then forget the counter-hit window. */
    @Override protected void onDisable() {
        release();
        windowOpen = false;
        reduceArmed = false;
        windowTicks = 0;
    }

    /** OpenOnyx handleEventSub613: a world change drops the backlog instead of replaying it. */
    @EventTarget private void onRespawn(RespawnEvent event) { reset(); }

    /** 1.8 read EntityPlayerSP.isInWeb; 26.2 only exposes the stuck multiplier through this hook. */
    @EventTarget private void onStuckInBlock(StuckInBlockEvent event) { stuck = true; }

    @EventTarget private void onPacket(PacketEvent event) {
        if (event.getType() != EventType.INCOMING || event.isCancelled()) return;
        Packet<?> packet = event.getPacket();
        // This runs on the netty thread. mc.player is cleared by the client thread on disconnect
        // and on a world change, so read it once: checking it and then dereferencing it again is a
        // race, and the NPE escapes channelRead0 as netty's "Internal Exception" disconnect.
        var player = mc.player;
        // While holding, everything behind the knockback is swallowed as well: that is the Lag mode.
        if (holding && swallow(packet, event)) return;
        if (packet instanceof ClientboundSetEntityMotionPacket motion) {
            if (player == null || motion.id() != player.getId()) return;
            // JumpReset is absent, so its "only off the ground" clause is always satisfied.
            if (delayKnockback.get() && meleeHit) {
                if (explosion) {
                    explosion = false;
                } else {
                    held.add(packet);
                    heldTicks = 0;
                    holding = true;
                    event.setCancelled(true);
                    // The knockback has not landed yet, so the counter-hit window opens on replay.
                    return;
                }
            }
            armReduce();
        } else if (packet instanceof ClientboundDamageEventPacket damage) {
            // 1.8 watched S19PacketEntityStatus opcode 2; 26.2 moved the hurt event to its own packet.
            if (player != null && damage.entityId() == player.getId()) meleeHit = true;
        } else if (packet instanceof ClientboundExplodePacket explode && hasKnockback(explode)) {
            // 26.2 applies explosion knockback from this packet, so it is the knockback itself.
            explosion = true;
        }
    }

    /** OpenOnyx isPacket. Returns true when the packet was queued and cancelled. */
    private boolean swallow(Packet<?> packet, PacketEvent event) {
        var player = mc.player; // Netty thread: snapshot it, see onPacket.
        if (packet instanceof ClientboundKeepAlivePacket) return false;
        if (packet instanceof ClientboundLoginPacket || packet instanceof ClientboundRespawnPacket) {
            flush();
            return false;
        }
        // Only our own hurt event is worth holding back; other entities keep updating.
        if (packet instanceof ClientboundDamageEventPacket damage
                && (player == null || damage.entityId() != player.getId())) return false;
        held.add(packet);
        event.setCancelled(true);
        return true;
    }

    /** OpenOnyx handleEventSub3: knockback landed, so the counter-hit may fire for a few ticks. */
    private void armReduce() {
        if (explosion) {
            explosion = false;
            return;
        }
        windowOpen = true;
        reduceArmed = reduceKnockback.get();
        windowTicks = 0;
    }

    @EventTarget private void onGameTick(GameTickEvent event) {
        if (event.getType() != EventType.PRE) return;
        stuck = false;
        if (mc.player == null || mc.level == null) {
            reset();
            return;
        }
        targetNear = hasTarget();
        if (windowOpen && ++windowTicks >= reduceWindow.get()) {
            windowOpen = reduceArmed = false;
            windowTicks = 0;
        }
        if (pauseBackpedaling.get() && mc.options.keyDown.isDown()) release();
        if (!targetNear) release();
        if (holding) {
            heldTicks++;
            if (heldTicks >= heldLimit
                    || (releaseOnGround.get() && mc.player.onGround())
                    || (releaseAfterReduce.get() && reduced)
                    || mc.player.isInWater() || mc.player.isInLava()) release();
        }
    }

    /**
     * OpenOnyx handleEventSub103. The attack goes out as a bare packet before the position packet,
     * so there is no client side damage or crit prediction, exactly like the source.
     */
    @EventTarget private void onMotion(MotionEvent event) {
        if (event.getType() != EventType.PRE || !reduceArmed || !windowOpen) return;
        if (mc.player == null || mc.level == null || mc.getConnection() == null) return;
        Player target = lookingAtPlayer(REDUCE_REACH);
        if (target == null || !allowsReduce() || !mc.player.isSprinting() || stuck || !killAuraIdle()) return;
        mc.player.swing(InteractionHand.MAIN_HAND);
        mc.getConnection().send(new ServerboundAttackPacket(target.getId()));
        Vec3 movement = mc.player.getDeltaMovement();
        mc.player.setDeltaMovement(movement.x * .6, movement.y, movement.z * .6);
        mc.player.setSprinting(false);
        reduced = true;
        reduceArmed = false;
    }

    /** OpenOnyx isEnabled109. */
    private boolean allowsReduce() {
        if (ThreadLocalRandom.current().nextDouble(100) >= chance.get()) return false;
        if (pauseBackpedaling.get() && mc.options.keyDown.isDown()) return false;
        if (!targetNear) return false;
        return !onlyWhileMoving.get() || hasMoveInput();
    }

    /** OpenOnyx ForwardsBackwardsRecord.isEnabled over the current movement input. */
    private boolean hasMoveInput() {
        Input keys = mc.player.input.keyPresses;
        return keys.forward() != keys.backward() || keys.left() != keys.right();
    }

    /** OpenOnyx run144: replay the backlog and forget this knockback. */
    private void release() {
        heldLimit = delayTicks.get();
        if (holding) flush();
        heldTicks = 0;
        meleeHit = false;
        explosion = false;
        reduced = false;
    }

    /** OpenOnyx run142. */
    private void flush() {
        holding = false;
        if (held.isEmpty()) return;
        List<Packet<?>> pending = new ArrayList<>(held);
        held.clear();
        // 1.8 called processPacket straight from whatever thread ran isPacket. 26.2 handlers go
        // through PacketUtils.ensureRunningOnSameThread, so a netty thread replay would be dropped.
        if (mc.isSameThread()) replay(pending);
        else mc.execute(() -> replay(pending));
    }

    private void replay(List<Packet<?>> pending) {
        ClientPacketListener listener = mc.getConnection();
        if (listener == null) return;
        for (Packet<?> packet : pending) {
            try {
                handle(packet, listener);
            } catch (RunningOnDifferentThreadException ignored) {
                // Vanilla rescheduled it; nothing left to do here.
            } catch (RuntimeException exception) {
                Myth.LOGGER.error("Velocity could not replay {}", packet.getClass().getSimpleName(), exception);
            }
            if (packet instanceof ClientboundSetEntityMotionPacket motion
                    && mc.player != null && motion.id() == mc.player.getId()) armReduce();
        }
    }

    @SuppressWarnings({"unchecked", "rawtypes"})
    private static void handle(Packet<?> packet, ClientPacketListener listener) {
        ((Packet) packet).handle(listener);
    }

    /** OpenOnyx run143: a full reset that keeps nothing, used when the world goes away. */
    private void reset() {
        held.clear();
        holding = false;
        heldTicks = 0;
        meleeHit = false;
        explosion = false;
        windowOpen = false;
        windowTicks = 0;
        reduceArmed = false;
        reduced = false;
        targetNear = false;
        heldLimit = delayTicks.get();
    }

    /** OpenOnyx PlayersTeammatesEnum.isDouble11: any selected target inside the range and the FOV. */
    private boolean hasTarget() {
        Vec3 eye = mc.player.getEyePosition(1);
        double rangeSqr = TARGET_RANGE * TARGET_RANGE;
        for (Entity entity : mc.level.entitiesForRendering()) {
            if (!(entity instanceof LivingEntity living) || living == mc.player || !living.isAlive()) continue;
            if (living instanceof ArmorStand) continue;
            if (living instanceof Player player && player.isSpectator()) continue;
            if (!isTarget(living)) continue;
            if (living.getBoundingBox().distanceToSqr(eye) > rangeSqr) continue;
            if (inFov(living)) return true;
        }
        return false;
    }

    /**
     * OpenOnyx isEntityLivingBase9, minus its AntiBot filter and the two Hypixel teammate
     * heuristics; the scoreboard team is the part 26.2 answers on its own. Velocity only ever
     * offered Players and Teammates, so everything else is skipped.
     */
    private boolean isTarget(LivingEntity living) {
        if (!(living instanceof Player player)) return false;
        return mc.player.isAlliedTo(player) ? targetTeammates.get() : targetPlayers.get();
    }

    /** OpenOnyx isEntity3: a yaw only cone, compared against half the setting. */
    private boolean inFov(Entity entity) {
        if (fov.get() >= 360) return true;
        double dx = entity.getX() - mc.player.getX();
        double dz = entity.getZ() - mc.player.getZ();
        float yaw = (float) Math.toDegrees(Math.atan2(-dx, dz));
        return Math.abs(Mth.wrapDegrees(yaw - mc.player.getYRot())) <= fov.get() / 2f;
    }

    /** OpenOnyx getEntityPlayer2: the nearest player the real crosshair ray reaches. */
    private Player lookingAtPlayer(double range) {
        Vec3 eye = mc.player.getEyePosition(1);
        Vec3 end = eye.add(mc.player.getLookAngle().scale(range));
        Player best = null;
        double nearest = Double.MAX_VALUE;
        for (Player candidate : mc.level.players()) {
            if (candidate == mc.player || !candidate.isAlive() || candidate.isSpectator()) continue;
            AABB box = candidate.getBoundingBox().inflate(candidate.getPickRadius());
            if (box.contains(eye)) return candidate;
            Optional<Vec3> hit = box.clip(eye, end);
            if (hit.isEmpty()) continue;
            double distance = eye.distanceTo(hit.get());
            if (distance < nearest) {
                best = candidate;
                nearest = distance;
            }
        }
        return best;
    }

    /**
     * Stands in for OpenOnyx KillAuraModule.isEnabled112, which read four flags of its KillAura
     * attack state machine that the renamer left unreadable. What it guards against is two attack
     * packets in one tick, so this yields whenever KillAura is holding a target.
     */
    private boolean killAuraIdle() {
        return Myth.MODEL_MANAGER.getModel(KillAura.class)
                .map(aura -> !aura.isEnabled() || KillAura.getTarget() == null).orElse(true);
    }

    private static boolean hasKnockback(ClientboundExplodePacket packet) {
        return packet.playerKnockback().filter(knockback -> knockback.lengthSqr() != 0).isPresent();
    }
}
