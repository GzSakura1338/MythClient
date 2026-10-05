package today.starain.module.impl.movement;

import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.phys.Vec3;
import today.starain.event.api.EventType;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.MotionEvent;
import today.starain.event.impl.MoveInputEvent;
import today.starain.event.impl.StuckInBlockEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.value.impl.NumberValue;

public class FastWeb extends ClientModule {
    private final NumberValue groundMultiplier = new NumberValue("Ground Multiplier", 0.0F, 1.0F, 0.66F, 0.001F, this);

    private int playerInWebTick = -1;
    private int lastGroundSneakTick = -100;
    private int inputTick = -1;
    private float inputForward = 0.0F;
    private float inputStrafe = 0.0F;
    private boolean inputJump = false;
    private boolean inputSneak = false;

    public FastWeb() {
        setName("FastWeb");
        setCategory(Category.MOVEMENT);
    }

    @EventTarget
    public void onMoveInput(MoveInputEvent event) {
        if (mc.player == null) {
            return;
        }

        int tick = mc.player.tickCount;
        if (tick != this.playerInWebTick) {
            return;
        }

        this.inputTick = tick;
        this.inputForward = event.getForward();
        this.inputStrafe = event.getStrafe();
        this.inputJump = event.isJump();
        this.inputSneak = event.isSneak();

        if (!mc.player.onGround()) {
            return;
        }

        if (event.isSneak()) {
            this.lastGroundSneakTick = mc.player.tickCount;
        }

        if (!event.isSneak() && (mc.player.tickCount - this.lastGroundSneakTick) > 1) {
            return;
        }

        event.setJump(false);
    }

    @EventTarget
    public void onMotion(MotionEvent e) {
        if (mc.player == null) {
            return;
        }

        if (e.getType() != EventType.PRE) {
            return;
        }

        int tick = mc.player.tickCount;
        if (tick != this.playerInWebTick) {
            return;
        }

        Vec3 base = mc.player.getDeltaMovement();
        boolean onGround = mc.player.onGround();

        float forward = 0.0F;
        float strafe = 0.0F;
        boolean jump = false;
        boolean sneak = false;

        if (mc.player.input != null) {
            forward = mc.player.input.forwardImpulse;
            strafe = mc.player.input.leftImpulse;
            jump = mc.player.input.jumping;
            sneak = mc.player.input.shiftKeyDown;
        } else if (this.inputTick == tick) {
            forward = this.inputForward;
            strafe = this.inputStrafe;
            jump = this.inputJump;
            sneak = this.inputSneak;
        }

        if (forward != 0 && strafe != 0) {
            forward *= 0.707f;
            strafe *= 0.707f;
        }

        double motionX;
        double motionZ;
        if (onGround) {
            motionX = base.x;
            motionZ = base.z;
        } else {
            motionX = 0.0;
            motionZ = 0.0;
            if (forward != 0 || strafe != 0) {
                double speed = 0.14122;
                float yaw = mc.player.getYRot();
                double radYaw = Math.toRadians(yaw);
                motionX = (-Math.sin(radYaw) * forward + Math.cos(radYaw) * strafe) * speed;
                motionZ = (Math.cos(radYaw) * forward + Math.sin(radYaw) * strafe) * speed;
            }
        }

        double motionY;
        if (onGround) {
            motionY = base.y;
        } else if (jump && sneak) {
            motionY = 0.0;
        } else if (jump) {
            motionY = 0.06222;
        } else if (sneak) {
            motionY = -0.18777F;
        } else {
            motionY = 0.0;
        }

        mc.player.setDeltaMovement(motionX, motionY, motionZ);
        mc.player.fallDistance = 0.0F;
    }

    @EventTarget
    public void onStuck(StuckInBlockEvent e) {
        if (mc.player == null) {
            return;
        }
        if (e.getState().getBlock() == Blocks.COBWEB) {
            this.playerInWebTick = mc.player.tickCount;
            Vec3 vanilla = e.getStuckSpeedMultiplier();
            if (mc.player.onGround()) {
                double t = this.groundMultiplier.get().doubleValue();
                double x = vanilla.x + (1.0 - vanilla.x) * t;
                double z = vanilla.z + (1.0 - vanilla.z) * t;
                e.setStuckSpeedMultiplier(new Vec3(x, vanilla.y, z));
            } else {
                e.setStuckSpeedMultiplier(new Vec3(1.0, 1.0, 1.0));
            }
        }
    }
}
