package today.starain.utils.rotation;

import lombok.Getter;
import lombok.Setter;
import net.minecraft.network.protocol.game.ServerboundMovePlayerPacket.PosRot;
import net.minecraft.util.Mth;
import today.starain.event.api.EventType;
import today.starain.event.api.Priority;
import today.starain.event.api.annotations.EventPriority;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.*;
import today.starain.utils.player.MoveUtils;
import today.starain.utils.wrapper.Wrapper;

import java.util.function.Function;

public class RotationManager implements Wrapper {
    private static final Rotation offset = new Rotation(0, 0);
    public static Rotation rotations, lastRotations, targetRotations, animationRotation, lastAnimationRotation;
    @Setter
    @Getter
    private static boolean active, smoothed;
    private static double rotationSpeed;
    private static Function<Rotation, Boolean> raycast;
    private static float randomAngle;

    public static void setRotations(final Rotation rotations, final double rotationSpeed) {
        setRotations(rotations, rotationSpeed, null);
    }

    public static void setRotations(final Rotation rotations, final double rotationSpeed, final Function<Rotation, Boolean> raycast) {
        RotationManager.targetRotations = new Rotation(rotations.getYaw(), clampPitch(rotations.getPitch()));
        RotationManager.rotationSpeed = rotationSpeed;
        RotationManager.raycast = raycast;
        active = true;

        smooth();
    }

    public static void smooth() {
        try {
            if (!smoothed) {
                float targetYaw = targetRotations.getYaw();
                float targetPitch = clampPitch(targetRotations.getPitch());

                if (raycast != null && (Math.abs(targetYaw - rotations.getYaw()) > 5 || Math.abs(targetPitch - rotations.getPitch()) > 5)) {
                    final Rotation trueTargetRotations = new Rotation(targetRotations.getYaw(), targetRotations.getPitch());

                    double speed = /*Math.min(*/(Math.random() * Math.random() * Math.random()) * 20/*, MoveUtil.speed() * 30)*/;
                    randomAngle += (float) ((20 + (float) (Math.random() - 0.5) * (Math.random() * Math.random() * Math.random() * 360)) * (mc.player.tickCount / 10 % 2 == 0 ? -1 : 1));

                    offset.setYaw((float) (offset.getYaw() + -Mth.sin((float) Math.toRadians(randomAngle)) * speed));
                    offset.setPitch((float) (offset.getPitch() + Mth.cos((float) Math.toRadians(randomAngle)) * speed));

                    targetYaw += offset.getYaw();
                    targetPitch = clampPitch(targetPitch + offset.getPitch());

                    if (!raycast.apply(new Rotation(targetYaw, targetPitch))) {
                        randomAngle = (float) Math.toDegrees(Math.atan2(trueTargetRotations.getYaw() - targetYaw, targetPitch - trueTargetRotations.getPitch())) - 180;

                        targetYaw -= offset.getYaw();
                        targetPitch -= offset.getPitch();

                        offset.setYaw((float) (offset.getYaw() + -Mth.sin((float) Math.toRadians(randomAngle)) * speed));
                        offset.setPitch((float) (offset.getPitch() + Mth.cos((float) Math.toRadians(randomAngle)) * speed));

                        targetYaw = targetYaw + offset.getYaw();
                        targetPitch = clampPitch(targetPitch + offset.getPitch());
                    }

                    if (!raycast.apply(new Rotation(targetYaw, targetPitch))) {
                        offset.setYaw(0);
                        offset.setPitch(0);

                        targetYaw = (float) (targetRotations.getYaw() + Math.random() * 2);
                        targetPitch = clampPitch((float) (targetRotations.getPitch() + Math.random() * 2));
                    }
                }

                rotations = RotationUtils.smooth(new Rotation(targetYaw, targetPitch),
                        rotationSpeed + Math.random());

            }

            smoothed = true;

            mc.gameRenderer.pick(1.0F);
        } catch (Exception e) {

        }
    }

    public static Rotation getRotation() {
        return active ? rotations : new Rotation(mc.player.getYRot(), mc.player.getXRot());
    }

    private static float clampPitch(float pitch) {
        return Mth.clamp(pitch, -90.0f, 90.0f);
    }

    public static RotationManager newInstance() {
        return new RotationManager();
    }

    @EventTarget
    public void onRespawn(RespawnEvent e) {
        lastRotations = null;
        rotations = null;
    }

    @EventPriority(Priority.LOW)
    @EventTarget()
    public void updateGlobalYaw(PlayerTickEvent e) {
        if (mc.player != null) {

            if (!active || rotations == null || lastRotations == null || targetRotations == null) {
                rotations = lastRotations = targetRotations = new Rotation(mc.player.getYRot(), mc.player.getXRot());
            }

            if (active) {
                smooth();
            }
        }
    }

    @EventTarget
    public void onAnimation(RotationAnimationEvent e) {
        if (active && animationRotation != null && lastAnimationRotation != null) {
            e.setYaw(animationRotation.getYaw());
            e.setLastYaw(lastAnimationRotation.getYaw());
            e.setPitch(animationRotation.getPitch());
            e.setLastPitch(lastAnimationRotation.getPitch());
        }
    }

    @EventPriority(Priority.LOWEST)
    @EventTarget()
    public void onPre(MotionEvent e) {
        if (e.getType() == EventType.PRE) {
            if (active && rotations != null) {

                float yaw = rotations.getYaw();
                float pitch = clampPitch(rotations.getPitch());
                if (!Float.isNaN(yaw) && !Float.isNaN(pitch) && active) {
                    e.setYaw(yaw);
                    e.setPitch(pitch);
                }

                if (Math.abs((rotations.getYaw() - mc.player.getYRot()) % 360) < 1 && Math.abs((rotations.getPitch() - mc.player.getXRot())) < 1) {
                    active = false;

                    this.correctDisabledRotations();
                }

                lastRotations = rotations;
            } else {
                lastRotations = new Rotation(mc.player.getYRot(), mc.player.getXRot());
            }

            lastAnimationRotation = animationRotation;
            animationRotation = new Rotation(e.getYaw(), e.getPitch());
            targetRotations = new Rotation(mc.player.getYRot(), mc.player.getXRot());
            raycast = null;
            smoothed = false;
        }
    }

    @EventPriority(Priority.HIGH)
    @EventTarget()
    public void onMove(MoveInputEvent event) {
        if (active && rotations != null) {
            float yaw = rotations.getYaw();
            MoveUtils.fixMovement(event, yaw);
        }
    }

    @EventTarget
    public void onMove(RayTraceEvent event) {
        if (rotations != null && event.entity == mc.player && active) {
            event.setYaw(rotations.getYaw());
            event.setPitch(rotations.getPitch());
        }
    }

    @EventTarget
    public void onItemRayTrace(UseItemRayTraceEvent event) {
        if (rotations != null && active) {
            event.setYaw(rotations.getYaw());
            event.setPitch(rotations.getPitch());
        }
    }

    @EventTarget
    public void onStrafe(StrafeEvent event) {
        if (active && rotations != null) {
            event.setYaw(rotations.getYaw());
        }
    }

    @EventTarget
    public void onJump(JumpEvent event) {
        if (active && rotations != null) {
            event.setYaw(rotations.getYaw());
        }
    }

    @EventPriority(Priority.HIGHEST)
    @EventTarget
    public void onPositionItem(PositionItemEvent e) {
        if (active && rotations != null) {
            PosRot packet = (PosRot) e.getPacket();
            float pitch = clampPitch(rotations.getPitch());
            PosRot newPacket = new PosRot(packet.getX(0.0), packet.getY(0.0), packet.getZ(0.0), rotations.getYaw(), pitch, packet.isOnGround());
            e.setPacket(newPacket);
        }
    }

    @EventTarget
    public void onFallFlying(FallFlyingEvent e) {
        if (rotations != null) {
            e.setPitch(rotations.getPitch());
        }
    }

    @EventTarget
    public void onAttack(AttackYawEvent e) {
        if (rotations != null) {
            e.setYaw(rotations.getYaw());
        }
    }

    private void correctDisabledRotations() {
        final Rotation rotations = new Rotation(mc.player.getYRot(), mc.player.getXRot());
        final Rotation fixedRotations = RotationUtils.resetRotation(RotationUtils.applySensitivityPatch(rotations, lastRotations));

        mc.player.setYRot(fixedRotations.getYaw());
        mc.player.setXRot(fixedRotations.getPitch());
    }

}
