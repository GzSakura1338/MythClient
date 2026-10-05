package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import com.llamalad7.mixinextras.injector.v2.WrapWithCondition;
import com.mojang.authlib.GameProfile;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.Feature_14;
import me.mioclient.KeyPearlMode;
import me.mioclient.MixinPlayerEntityData;
import me.mioclient.NameTagsHelper;
import me.mioclient.NameTagsHelperMode;
import me.mioclient.PhaseESPSearchHelper4_2;
import me.mioclient.SearchHelper_4;
import me.mioclient.event.AttackHookPostEvent;
import me.mioclient.event.ClipAtLedgeEvent;
import me.mioclient.module.exploit.Reach;
import me.mioclient.module.movement.Sprint;
import me.mioclient.module.player.Scaffold;
import net.minecraft.entity.Entity;
import net.minecraft.entity.EntityType;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.BlockItem;
import net.minecraft.util.Hand;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Vec3d;
import net.minecraft.util.shape.VoxelShape;
import net.minecraft.world.World;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({PlayerEntity.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinPlayerEntity.class */
public abstract class MixinPlayerEntity extends LivingEntity implements NameTagsHelper {
    private static final Scaffold scaffold = (Scaffold) BaritoneHelper_3.baritoneHelper_4.getModule117(Scaffold.class);
    private static final Sprint sprint = (Sprint) BaritoneHelper_3.baritoneHelper_4.getModule117(Sprint.class);
    private static final Reach reach = (Reach) BaritoneHelper_3.baritoneHelper_4.getModule117(Reach.class);

    @Unique
    private long mio$lastEatingTime;

    @Unique
    private NameTagsHelperMode mio$role;

    @Unique
    private boolean mio$isNextToWall;

    protected MixinPlayerEntity(EntityType<? extends LivingEntity> entityType, World world) {
        super(entityType, world);
    }

    @Inject(method = {"<init>"}, at = {@At("TAIL")})
    private void initHook(World world, BlockPos blockPos, float f, GameProfile gameProfile, CallbackInfo callbackInfo) {
        for (MixinPlayerEntityData mixinPlayerEntityData : BaritoneHelper_3.searchHelper4_14.getRegistry()) {
            if (gameProfile.getName().equalsIgnoreCase(mixinPlayerEntityData.getString333())) {
                mio$setRole(mixinPlayerEntityData.getNameTagsHelperMode631());
                return;
            }
        }
    }

    @Inject(method = {"tick"}, at = {@At("HEAD")})
    private void tick(CallbackInfo callbackInfo) {
        this.mio$isNextToWall = !getWorld().isSpaceEmpty(getBoundingBox().expand(1.0E-4d, 0.0d, 1.0E-4d));
        if (!isUsingItem() || get70(getActiveItem(), this) > 1.0f) {
            this.mio$lastEatingTime = System.currentTimeMillis();
        }
    }

    @Inject(method = {"clipAtLedge"}, at = {@At("HEAD")}, cancellable = true)
    private void clipAtLedgeHook(CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        if (SearchHelper_4.minecraftClient.player == null) {
            return;
        }
        ClipAtLedgeEvent clipAtLedgeEvent = new ClipAtLedgeEvent();
        SearchHelper_4.baritoneHelper.getObject1794(clipAtLedgeEvent);
        if (clipAtLedgeEvent.is2403()) {
            callbackInfoReturnable.setReturnValue(true);
        }
    }

    @ModifyExpressionValue(method = {"adjustMovementForSneaking"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/entity/player/PlayerEntity;getStepHeight()F")})
    private float clipAtLedgeHook2(float f) {
        if (!scaffold.isToggled() || !scaffold.safeWalk.getValue().booleanValue() || SearchHelper_4.minecraftClient.player == null) {
            return f;
        }
        // 反编译把转型移到了 instanceof 检查之前——空手时 getItem() 是 AirBlockItem(非 BlockItem),
        // 未检查就强转 → ClassCastException。必须先 instanceof 再转。
        net.minecraft.item.Item rawItem = SearchHelper_4.minecraftClient.player.getStackInHand(Hand.MAIN_HAND).getItem();
        if (!(rawItem instanceof BlockItem)) {
            return f;
        }
        BlockItem item = (BlockItem) rawItem;
        VoxelShape collisionShape = item.getBlock().getDefaultState().getCollisionShape(SearchHelper_4.minecraftClient.world, BlockPos.ORIGIN);
        return (collisionShape == null || collisionShape.isEmpty()) ? f : (float) collisionShape.getBoundingBox().getLengthY();
    }

    @Inject(method = {"attack"}, at = {@At("HEAD")}, cancellable = true)
    public void attackHookPre(Entity entity, CallbackInfo callbackInfo) {
        AttackHookPostEvent attackHookPostEvent = new AttackHookPostEvent(KeyPearlMode.Pre, entity);
        SearchHelper_4.baritoneHelper.getObject1794(attackHookPostEvent);
        if (attackHookPostEvent.is2403()) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"attack"}, at = {@At("TAIL")})
    private void attackHookPost(Entity entity, CallbackInfo callbackInfo) {
        SearchHelper_4.baritoneHelper.getObject1794(new AttackHookPostEvent(KeyPearlMode.Post, entity));
    }

    @WrapWithCondition(method = {"attack"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/entity/player/PlayerEntity;setVelocity(Lnet/minecraft/util/math/Vec3d;)V")})
    private boolean attackHook(PlayerEntity playerEntity, Vec3d vec3d) {
        return (sprint.isToggled() && sprint.keep.getValue().booleanValue()) ? false : true;
    }

    @WrapWithCondition(method = {"attack"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/entity/LivingEntity;takeKnockback(DDD)V")})
    private boolean attack(LivingEntity livingEntity, double d, double d2, double d3) {
        return (((((Object) this) instanceof Feature_14.OtherClientPlayerEntity) && livingEntity == SearchHelper_4.minecraftClient.player) ? false : true);
    }

    @Inject(method = {"getEntityInteractionRange"}, at = {@At("HEAD")}, cancellable = true)
    private void getEntityInteractionRangeHook(CallbackInfoReturnable<Double> callbackInfoReturnable) {
        if (reach.isToggled()) {
            callbackInfoReturnable.setReturnValue(Double.valueOf(reach.modifier.getValue().doubleValue()));
        }
    }

    @Inject(method = {"getBlockInteractionRange"}, at = {@At("HEAD")}, cancellable = true)
    private void getBlockInteractionRangeHook(CallbackInfoReturnable<Double> callbackInfoReturnable) {
        if (reach.isToggled()) {
            callbackInfoReturnable.setReturnValue(Double.valueOf(reach.modifier.getValue().doubleValue()));
        }
    }

    @Inject(method = {"shouldCancelInteraction"}, at = {@At("HEAD")}, cancellable = true)
    private void shouldCancelInteractionHook(CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        if (PhaseESPSearchHelper4_2.flag) {
            callbackInfoReturnable.setReturnValue(true);
        }
    }

    @Override // me.mioclient.NameTagsHelper
    public long mio$getLastEatingTime() {
        return this.mio$lastEatingTime;
    }

    @Override // me.mioclient.NameTagsHelper
    public NameTagsHelperMode mio$getRole() {
        return this.mio$role;
    }

    @Override // me.mioclient.NameTagsHelper
    public void mio$setRole(NameTagsHelperMode nameTagsHelperMode) {
        this.mio$role = nameTagsHelperMode;
    }

    @Override // me.mioclient.NameTagsHelper
    public boolean mio$isNextToWall() {
        return this.mio$isNextToWall;
    }
}
