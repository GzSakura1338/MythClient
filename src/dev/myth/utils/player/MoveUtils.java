package dev.myth.utils.player;

import net.minecraft.client.Minecraft;
import dev.myth.access.MovementInputAccess;
import net.minecraft.util.Mth;
import net.minecraft.world.phys.Vec3;
import dev.myth.event.impl.MoveInputEvent;

public class MoveUtils {
    private static final Minecraft mc = Minecraft.getInstance();

    public static boolean isMoving() {
        if (mc.player == null) return false;
        return MovementInputAccess.forwardImpulse(mc.player.input) != 0.0F || MovementInputAccess.leftImpulse(mc.player.input) != 0.0F;
    }

    public static double getSpeed() {
        if (mc.player == null) return 0.0;
        return Math.sqrt(
                mc.player.getDeltaMovement().x * mc.player.getDeltaMovement().x +
                        mc.player.getDeltaMovement().z * mc.player.getDeltaMovement().z
        );
    }

    public static void setSpeed(double speed) {
        if (mc.player == null) return;

        Vec3 motion = mc.player.getDeltaMovement();
        float yaw = mc.player.getYRot();

        double forward = MovementInputAccess.forwardImpulse(mc.player.input);
        double strafe = MovementInputAccess.leftImpulse(mc.player.input);

        if (forward != 0.0 || strafe != 0.0) {
            if (forward != 0.0) {
                if (strafe > 0.0) {
                    yaw += (float) (forward > 0.0 ? -45 : 45);
                } else if (strafe < 0.0) {
                    yaw += (float) (forward > 0.0 ? 45 : -45);
                }

                strafe = 0.0;
                if (forward > 0.0) {
                    forward = 1.0;
                } else if (forward < 0.0) {
                    forward = -1.0;
                }
            }

            double cos = Math.cos(Math.toRadians(yaw + 90.0F));
            double sin = Math.sin(Math.toRadians(yaw + 90.0F));

            mc.player.setDeltaMovement(
                    forward * speed * cos + strafe * speed * sin,
                    motion.y,
                    forward * speed * sin - strafe * speed * cos
            );
        } else {
            mc.player.setDeltaMovement(0, motion.y, 0);
        }
    }

    public static double getDirection() {
        if (mc.player == null) return 0.0;

        float yaw = mc.player.getYRot();
        float forward = MovementInputAccess.forwardImpulse(mc.player.input);
        float strafe = MovementInputAccess.leftImpulse(mc.player.input);

        if (forward < 0.0f) {
            yaw += 180.0f;
        }

        float strafeAngle = 0.0f;
        if (strafe < 0.0f) {
            strafeAngle = -90.0f;
        } else if (strafe > 0.0f) {
            strafeAngle = 90.0f;
        }

        if (forward != 0.0f) {
            if (strafe >= 0.0f) {
                yaw += strafeAngle * (forward > 0.0f ? 0.5f : -0.5f);
            } else {
                yaw -= strafeAngle * (forward > 0.0f ? 0.5f : -0.5f);
            }
        }

        return Math.toRadians(yaw);
    }


    public static void fixMovement(MoveInputEvent event, float yaw) {
        float forward = event.getForward();
        float strafe = event.getStrafe();
        int angleUnit = 45;
        float angleTolerance = 22.5F;
        float directionFactor = Math.max(Math.abs(forward), Math.abs(strafe));
        double angleDifference = Mth.wrapDegrees(direction(forward, strafe) - yaw);
        double angleDistance = Math.abs(angleDifference);
        forward = 0.0F;
        strafe = 0.0F;
        if (angleDistance <= (double) ((float) angleUnit + angleTolerance)) {
            forward++;
        } else if (angleDistance >= (double) (180.0F - (float) angleUnit - angleTolerance)) {
            forward--;
        }

        if (angleDifference >= (double) ((float) angleUnit - angleTolerance) && angleDifference <= (double) (180.0F - (float) angleUnit + angleTolerance)) {
            strafe--;
        } else if (angleDifference <= (double) ((float) (-angleUnit) + angleTolerance) && angleDifference >= (double) (-180.0F + (float) angleUnit - angleTolerance)) {
            strafe++;
        }

        forward *= directionFactor;
        strafe *= directionFactor;
        event.setForward(forward);
        event.setStrafe(strafe);
    }

    private static float direction(float forward, float strafe) {
        float direction = mc.player.getYRot();
        boolean isMovingForward = forward > 0.0F;
        boolean isMovingBack = forward < 0.0F;
        boolean isMovingRight = strafe > 0.0F;
        boolean isMovingLeft = strafe < 0.0F;
        boolean isMovingSideways = isMovingRight || isMovingLeft;
        boolean isMovingStraight = isMovingForward || isMovingBack;
        if (forward != 0.0F || strafe != 0.0F) {
            if (isMovingBack && !isMovingSideways) {
                return direction + 180.0F;
            }

            if (isMovingForward && isMovingLeft) {
                return direction + 45.0F;
            }

            if (isMovingForward && isMovingRight) {
                return direction - 45.0F;
            }

            if (!isMovingStraight && isMovingLeft) {
                return direction + 90.0F;
            }

            if (!isMovingStraight && isMovingRight) {
                return direction - 90.0F;
            }

            if (isMovingBack && isMovingLeft) {
                return direction + 135.0F;
            }

            if (isMovingBack) {
                return direction - 135.0F;
            }
        }

        return direction;
    }
}