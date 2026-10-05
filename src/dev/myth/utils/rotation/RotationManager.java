package dev.myth.utils.rotation;

import lombok.Getter;
import lombok.Setter;
import net.minecraft.network.protocol.game.ServerboundUseItemPacket;
import dev.myth.access.MinecraftAccess;
import net.minecraft.util.Mth;
import dev.myth.event.api.EventType;
import dev.myth.event.api.Priority;
import dev.myth.event.api.annotations.EventPriority;
import dev.myth.event.api.annotations.EventTarget;
import dev.myth.event.impl.*;
import dev.myth.utils.player.MoveUtils;
import dev.myth.utils.wrapper.Wrapper;

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
        if (mc.player == null || rotations == null || !Float.isFinite(rotations.getYaw())
                || !Float.isFinite(rotations.getPitch()) || !Double.isFinite(rotationSpeed) || rotationSpeed < 0) return;
        if (RotationManager.rotations == null || lastRotations == null || targetRotations == null) {
            RotationManager.rotations = lastRotations = new Rotation(mc.player.getYRot(), mc.player.getXRot());
        }
        RotationManager.targetRotations = new Rotation(rotations.getYaw(), clampPitch(rotations.getPitch()));
        RotationManager.rotationSpeed = rotationSpeed;
        RotationManager.raycast = raycast;
        active = true;

        smooth();
    }

    public static void smooth() {
        try {
            if (!smoothed) {
                float targetYaw = RotationUtils.relateToPlayerRotation(targetRotations, lastRotations).getYaw();
                float targetPitch = clampPitch(targetRotations.getPitch());

                if (raycast != null && (Math.abs(Mth.wrapDegrees(targetYaw - rotations.getYaw())) > 5 || Math.abs(targetPitch - rotations.getPitch()) > 5)) {
                    final Rotation trueTargetRotations = new Rotation(targetYaw, targetPitch);

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

                        targetYaw = (float) (trueTargetRotations.getYaw() + Math.random() * 2);
                        targetPitch = clampPitch((float) (targetRotations.getPitch() + Math.random() * 2));
                    }
                }

                rotations = RotationUtils.smooth(new Rotation(targetYaw, targetPitch),
                        rotationSpeed + Math.random());
                // Nova's final quantization uses the last published silent angle as origin.
                Rotation applied = RotationUtils.applySensitivityPatch(
                        RotationUtils.relateToPlayerRotation(rotations, lastRotations), lastRotations, false);
                rotations = Float.isFinite(applied.getYaw()) && Float.isFinite(applied.getPitch()) ? applied : lastRotations;

            }

            smoothed = true;

            ((MinecraftAccess) mc).myth$pick(1.0F);
        } catch (Exception e) {

        }
    }

    public static Rotation getRotation() {
        return active ? rotations : new Rotation(mc.player.getYRot(), mc.player.getXRot());
    }

    public static void setActive(boolean value) {
        if (!value && active) correctDisabledRotations();
        active = value;
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
        targetRotations = animationRotation = lastAnimationRotation = null;
        active = smoothed = false;
        raycast = null;
        offset.setYaw(0);
        offset.setPitch(0);
        randomAngle = 0;
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
                if (Float.isFinite(yaw) && Float.isFinite(pitch) && active) {
                    e.setYaw(yaw);
                    e.setPitch(pitch);
                } else {
                    onRespawn(new RespawnEvent());
                    return;
                }

                // Rebase against this motion's output, never the preceding tick.
                lastRotations = rotations;
                if (Math.abs(Mth.wrapDegrees(rotations.getYaw() - mc.player.getYRot())) < 1 && Math.abs((rotations.getPitch() - mc.player.getXRot())) < 1) {
                    setActive(false);
                }
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
            ServerboundUseItemPacket packet = (ServerboundUseItemPacket) e.getPacket();
            float pitch = clampPitch(rotations.getPitch());
            ServerboundUseItemPacket newPacket = new ServerboundUseItemPacket(packet.getHand(), packet.getSequence(), rotations.getYaw(), pitch);
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

    private static void correctDisabledRotations() {
        if (mc.player == null || lastRotations == null || !Float.isFinite(lastRotations.getYaw())) return;
        float cameraYaw = mc.player.getYRot();
        if (!Float.isFinite(cameraYaw) || !Float.isFinite(mc.player.getXRot())) return;
        // Nova: change only the whole-turn representation, preserving the live view.
        double turns = Math.round(((double) lastRotations.getYaw() - cameraYaw) / 360.0);
        if (turns == 0) return;
        float yaw = (float) (cameraYaw + turns * 360.0);
        if (!Float.isFinite(yaw)) return;
        float shift = yaw - cameraYaw;
        mc.player.setYRot(yaw);
        mc.player.yRotO += shift;
        // Both hand-sway interpolation endpoints must use the camera's revolution.
        mc.player.yBob += shift;
        mc.player.yBobO += shift;
    }

}
