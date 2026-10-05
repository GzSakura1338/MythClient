package today.starain.module.impl.combat;

import net.minecraft.client.multiplayer.ClientPacketListener;
import net.minecraft.network.protocol.Packet;
import net.minecraft.network.protocol.game.*;
import net.minecraft.util.Mth;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.phys.EntityHitResult;
import today.starain.Starain;
import today.starain.event.api.EventType;
import today.starain.event.api.Priority;
import today.starain.event.api.annotations.EventPriority;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.GameTickEvent;
import today.starain.event.impl.MoveInputEvent;
import today.starain.event.impl.PacketEvent;
import today.starain.event.impl.PlayerTickEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.module.impl.misc.AntiBot;
import today.starain.module.impl.player.Scaffold;
import today.starain.utils.rotation.Rotation;
import today.starain.utils.rotation.RotationManager;
import today.starain.value.impl.NumberValue;

import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;

public class Velocity extends ClientModule {

    public static int attackQueue;
    public static int sprintQueue;
    public static boolean delay;
    public static boolean jump;
    public static float yaw;
    private final NumberValue counts = new NumberValue("Attack Counts", 1, 5, 3, 1, this);
    private final NumberValue sprintTicks = new NumberValue("SprintTicks", 1, 5, 3, 1, this);
    private final NumberValue maxDelay = new NumberValue("Max Delay", 0, 1000, 1000, 50, this);
    private final Queue<Packet<? super ClientPacketListener>> packets = new ConcurrentLinkedQueue<>();
    private long lag;
    private long startDelay;

    public Velocity() {
        setName("Velocity");
        setCategory(Category.COMBAT);
    }

    private static float yawFromMotion(int motionX, int motionZ) {
        float yaw = (float) (Math.toDegrees(Math.atan2(motionZ, motionX)) + 90.0);
        return Mth.wrapDegrees(yaw);
    }

    @Override
    public void onDisable() {
        delay = false;
        packets.clear();
        attackQueue = 0;
        jump = false;
    }

    /**
     * 收到包时：在 delay 阶段拦截速度/位移相关的服务端包
     */
    @EventTarget
    public void onPacket(PacketEvent event) {
        try {
            if (event.getType() != EventType.INCOMING) return;
            Packet<?> packet = event.getPacket();

            if (packet instanceof ClientboundSetEntityMotionPacket
                    || packet instanceof ClientboundMoveEntityPacket
                    || packet instanceof ClientboundTeleportEntityPacket
                    || packet instanceof ClientboundPingPacket
                    || packet instanceof ClientboundPlayerLookAtPacket
                    || packet instanceof ClientboundPlayerPositionPacket) {
                if (delay) {
                    event.setCancelled(true);
                    packets.add((Packet<? super ClientPacketListener>) packet);
                }
            }

            if (packet instanceof ClientboundPlayerPositionPacket || packet instanceof ClientboundExplodePacket) {
                lag = System.currentTimeMillis();
            }

            if (packet instanceof ClientboundSetEntityMotionPacket motionPacket) {
                if (motionPacket.getId() == mc.player.getId() && System.currentTimeMillis() - lag >= 100) {
                    boolean chackFall = motionPacket.getYa() > 0 && (motionPacket.getXa() != 0 || motionPacket.getZa() != 0);
                    if (chackFall && !delay) {
                        delay = true;
                        event.setCancelled(true);
                        packets.add((Packet<? super ClientPacketListener>) packet);
                        startDelay = System.currentTimeMillis();
                    }
                    yaw = yawFromMotion(motionPacket.getXa(), motionPacket.getZa());
                }
            }
        } catch (Exception e) {

        }
    }

    @EventTarget
    public void onGameTick(GameTickEvent event) {
        if (event.getType() != EventType.PRE || mc.player == null) return;
        if (delay && (mc.player.onGround() || System.currentTimeMillis() - lag < 100 || System.currentTimeMillis() - startDelay >= maxDelay.get().intValue())) {
            flush();
            if (System.currentTimeMillis() - lag >= 100) {
                attackQueue = counts.getValue().intValue();
                sprintQueue = sprintTicks.get().intValue();
            }
        }
        if (sprintQueue >= 1 && (KillAura.targets == null || KillAura.targets.isEmpty()) && !Starain.getInstance().getModuleManager().getModule(Scaffold.class).isEnabled()) {
            RotationManager.setRotations(new Rotation(yaw, RotationManager.getRotation().getPitch()), 180f);
        }
    }

    @EventPriority(Priority.LOW)
    @EventTarget
    public void onPlayerTick(PlayerTickEvent event) {
        if (attackQueue >= 1) {
            if (mc.hitResult instanceof EntityHitResult entityHitResult && entityHitResult.getEntity() instanceof Player pl && !AntiBot.isBot(pl)) {
                mc.gameMode.attack(mc.player, pl);
                mc.player.swing(InteractionHand.MAIN_HAND);
            }
            attackQueue--;
        }
    }

    @EventPriority(Priority.LOW)
    @EventTarget
    public void onMoveInput(MoveInputEvent event) {
        if (delay && mc.player.fallDistance == 0 && mc.hitResult instanceof EntityHitResult entityHitResult && entityHitResult.getEntity() instanceof Player pl && !AntiBot.isBot(pl)) {
            if (System.currentTimeMillis() - lag >= 100) {
                event.setForward(1.0f);
                event.setStrafe(0.0f);
            }
        }

        if (jump && mc.player.onGround()) {
            if (System.currentTimeMillis() - lag >= 100) {
                event.setJump(true);
                event.setForward(1.0f);
                event.setStrafe(0.0f);
            }
            jump = false;
        }

        if (sprintQueue-- >= 1 && (KillAura.targets == null || KillAura.targets.isEmpty()) && !Starain.getInstance().getModuleManager().getModule(Scaffold.class).isEnabled() && System.currentTimeMillis() - lag >= 100) {
            event.setForward(1.0f);
            event.setStrafe(0.0f);
        }
    }

    private void flush() {
        try {
            if (!delay) return;
            while (!packets.isEmpty()) {
                Packet<? super ClientPacketListener> packet = packets.poll();
                if (packet != null && mc.getConnection() != null) {
                    packet.handle(mc.getConnection());
                }
            }
            delay = false;
            jump = true;
        } catch (Exception e) {

        }
    }

    @Override
    public String getSuffix() {
        return delay ? System.currentTimeMillis() - startDelay + "ms" : "";
    }
}
