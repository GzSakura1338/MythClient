package today.starain.module.impl.combat;

import net.minecraft.network.protocol.game.ServerboundSetCarriedItemPacket;
import net.minecraft.network.protocol.game.ServerboundUseItemPacket;
import net.minecraft.util.Mth;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.item.BlockItem;
import net.minecraft.world.item.EggItem;
import net.minecraft.world.item.EnderpearlItem;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.PotionItem;
import net.minecraft.world.item.ProjectileWeaponItem;
import net.minecraft.world.item.SnowballItem;
import net.minecraft.world.level.ClipContext;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.BlockHitResult;
import net.minecraft.world.phys.HitResult;
import net.minecraft.world.phys.Vec3;
import today.starain.Starain;
import today.starain.event.api.EventType;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.GameTickEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.module.impl.misc.AntiBot;
import today.starain.module.impl.misc.Target;
import today.starain.module.impl.misc.Teams;
import today.starain.module.impl.movement.Stuck;
import today.starain.module.impl.player.NoFall;
import today.starain.module.impl.player.Scaffold;
import today.starain.utils.player.FriendManager;
import today.starain.utils.rotation.Rotation;
import today.starain.utils.rotation.RotationManager;
import today.starain.utils.rotation.RotationUtils;
import today.starain.utils.timer.TimerUtils;
import today.starain.value.impl.NumberValue;

import java.util.List;

public class AutoThrow extends ClientModule {

    private final TimerUtils timer = new TimerUtils();
    private final NumberValue minDistance = new NumberValue("Min Distance", 0.0, 12.0, 2.0, 0.5, this);
    private final NumberValue maxDistance = new NumberValue("Max Distance", 1.0, 30.0, 10.0, 0.5, this);
    private final NumberValue fov = new NumberValue("FOV", 0.0, 360.0, 360.0, 1.0, this);
    private final NumberValue delay = new NumberValue("Delay", 0.0, 2000.0, 400.0, 10.0, this);
    private final NumberValue prediction = new NumberValue("Prediction", 0.0, 2.0, 1.0, 0.05, this);

    private int pendingSlot = -1;
    private int originalSlot = -1;
    private boolean pendingUse;
    private boolean pendingSwitchBack;
    private Rotation pendingRotation;

    public AutoThrow() {
        setName("AutoThrow");
        setCategory(Category.COMBAT);
    }

    @Override
    protected void onEnable() {
        timer.reset();
        resetPendingState(false);
    }

    @Override
    protected void onDisable() {
        resetPendingState(true);
    }

    @EventTarget
    public void onUpdate(GameTickEvent e) {
        if (e.getType() != EventType.PRE) return;
        if (mc.player == null || mc.level == null || mc.getConnection() == null) {
            resetPendingState(false);
            return;
        }
        if (mc.player.isUsingItem() || mc.screen != null || shouldYieldToBlockPlacing()
                || shouldSkipDueToHeldItem(mc.player.getMainHandItem()) || shouldYieldToScaffold()) {
            resetPendingState(true);
            return;
        }

        if (pendingUse) {
            if (shouldYieldToNoFall()) return;
            if (originalSlot != -1 && mc.player.getInventory().selected != originalSlot) {
                resetPendingState(false);
                return;
            }
            if (pendingRotation != null) {
                RotationManager.setRotations(pendingRotation, 180.0);
            }
            mc.getConnection().send(new ServerboundUseItemPacket(InteractionHand.MAIN_HAND,
                    mc.level.getBlockStatePredictionHandler().startPredicting().currentSequence()));
            pendingUse = false;
            pendingSwitchBack = true;
            return;
        }

        if (pendingSwitchBack) {
            if (originalSlot != -1) {
                mc.getConnection().send(new ServerboundSetCarriedItemPacket(originalSlot));
            }
            pendingSwitchBack = false;
            pendingSlot = -1;
            originalSlot = -1;
            pendingRotation = null;
            timer.reset();
            return;
        }

        if (!timer.delay(delay.get().longValue())) return;

        Player target = findTarget();
        if (target == null) return;

        int slot = findThrowableHotbarSlot();
        if (slot == -1) return;

        Rotation rotation = calculateThrowRotation(target);
        if (rotation == null) return;

        if (shouldYieldToNoFall()) return;
        if (Starain.getInstance().getModuleManager().getModule(Stuck.class).isEnabled()) return;

        RotationManager.setRotations(rotation, 180.0);
        originalSlot = mc.player.getInventory().selected;
        mc.getConnection().send(new ServerboundSetCarriedItemPacket(slot));
        pendingSlot = slot;
        pendingRotation = rotation;
        pendingUse = true;
    }

    private boolean shouldYieldToNoFall() {
        NoFall noFall = Starain.getInstance().getModuleManager().getModule(NoFall.class);
        return noFall != null && noFall.isEnabled() && noFall.isInPostRetrievePause();
    }

    private boolean shouldYieldToScaffold() {
        Scaffold scaffold = Starain.getInstance().getModuleManager().getModule(Scaffold.class);
        return scaffold != null && scaffold.isEnabled();
    }

    private boolean shouldYieldToBlockPlacing() {
        if (mc.player == null || mc.options == null || !mc.options.keyUse.isDown()) return false;
        ItemStack main = mc.player.getMainHandItem();
        if (!main.isEmpty() && main.getItem() instanceof BlockItem) return true;
        ItemStack off = mc.player.getOffhandItem();
        return !off.isEmpty() && off.getItem() instanceof BlockItem;
    }

    private Player findTarget() {
        if (mc.level == null || mc.player == null) return null;

        float range = maxDistance.get().floatValue();
        double rangeSq = range * range;
        float currentFov = fov.get().floatValue();

        AABB searchBox = mc.player.getBoundingBox().inflate(range);
        List<Entity> candidates = mc.level.getEntities(mc.player, searchBox,
                e -> e instanceof Player
                        && e != mc.player
                        && e.isAlive()
                        && !e.isSpectator()
                        && !AntiBot.isBot(e)
                        && Target.isTarget(e)
                        && !Teams.isSameTeam(e)
                        && !FriendManager.isFriend(e)
                        && Math.abs(Mth.wrapDegrees(RotationUtils.calculate(e).getYaw() - mc.player.getYRot())) <= currentFov);

        Player best = null;
        double minDstSq = Double.MAX_VALUE;
        for (Entity entity : candidates) {
            if (!hasLineOfSight(entity)) continue;
            double distSq = mc.player.distanceToSqr(entity);
            if (distSq > rangeSq) continue;

            double dx = entity.getX() - mc.player.getX();
            double dz = entity.getZ() - mc.player.getZ();
            if (Math.sqrt(dx * dx + dz * dz) < minDistance.get().floatValue()) continue;

            if (distSq < minDstSq) {
                minDstSq = distSq;
                best = (Player) entity;
            }
        }
        return best;
    }

    private boolean hasLineOfSight(Entity entity) {
        if (mc.player == null || mc.level == null || entity == null) return false;
        Vec3 start = mc.player.getEyePosition(1.0F);
        Vec3 end = entity.getEyePosition(1.0F);
        BlockHitResult hit = mc.level.clip(new ClipContext(start, end,
                ClipContext.Block.COLLIDER, ClipContext.Fluid.NONE, mc.player));
        return hit.getType() == HitResult.Type.MISS;
    }

    private int findThrowableHotbarSlot() {
        if (mc.player == null) return -1;
        for (int i = 0; i < 9; i++) {
            ItemStack stack = mc.player.getInventory().getItem(i);
            if (stack.isEmpty()) continue;
            Item item = stack.getItem();
            if (item instanceof SnowballItem || item instanceof EggItem) {
                return i;
            }
        }
        return -1;
    }

    private Rotation calculateThrowRotation(Player target) {
        if (mc.player == null) return null;

        double targetDx = target.getX() - target.xOld;
        double targetDy = target.getY() - target.yOld;
        double targetDz = target.getZ() - target.zOld;

        double px = mc.player.getX();
        double py = mc.player.getY() + mc.player.getEyeHeight();
        double pz = mc.player.getZ();

        double tx = target.getX();
        double ty = target.getY() + target.getEyeHeight() - 0.15;
        double tz = target.getZ();

        double dx = tx - px;
        double dz = tz - pz;
        double horizontalDist = Math.sqrt(dx * dx + dz * dz);
        double velocity = 1.5;
        double leadTicks = Mth.clamp((horizontalDist / velocity) * prediction.get().floatValue(), 0.0, 12.0);

        tx += targetDx * leadTicks;
        ty += targetDy * (leadTicks * 0.5);
        tz += targetDz * leadTicks;

        dx = tx - px;
        dz = tz - pz;
        horizontalDist = Math.sqrt(dx * dx + dz * dz);
        if (horizontalDist < 0.001) return null;

        double dy = ty - py;
        float yaw = (float) (Math.toDegrees(Math.atan2(dz, dx)) - 90.0);
        float pitch = calculateProjectilePitch(horizontalDist, dy, velocity, 0.03);
        return new Rotation(yaw, pitch);
    }

    private float calculateProjectilePitch(double horizontalDistance, double deltaY, double velocity, double gravity) {
        double v2 = velocity * velocity;
        double underSqrt = v2 * v2 - gravity * (gravity * horizontalDistance * horizontalDistance + 2.0 * deltaY * v2);
        double angleRad;
        if (underSqrt <= 0.0) {
            angleRad = Math.atan2(deltaY, horizontalDistance);
        } else {
            angleRad = Math.atan((v2 - Math.sqrt(underSqrt)) / (gravity * horizontalDistance));
        }
        float pitch = (float) -Math.toDegrees(angleRad);
        return Mth.clamp(pitch, -90.0F, 90.0F);
    }

    private boolean shouldSkipDueToHeldItem(ItemStack stack) {
        if (stack == null || stack.isEmpty()) return false;
        Item item = stack.getItem();
        if (item instanceof EnderpearlItem) return true;
        if (item instanceof ProjectileWeaponItem) return true;
        if (item instanceof PotionItem) return true;
        return stack.isEdible();
    }

    private void resetPendingState(boolean switchBack) {
        if (switchBack && originalSlot != -1 && mc.player != null && mc.getConnection() != null) {
            mc.getConnection().send(new ServerboundSetCarriedItemPacket(originalSlot));
        }
        pendingSlot = -1;
        originalSlot = -1;
        pendingUse = false;
        pendingSwitchBack = false;
        pendingRotation = null;
    }
}
