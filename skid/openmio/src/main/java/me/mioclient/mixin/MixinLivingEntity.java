package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import me.mioclient.AutoCrystalHelper_4;
import me.mioclient.AutoSignSearchHelper4;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.Feature_14;
import me.mioclient.HoleSnapSearchHelper4_3;
import me.mioclient.MixinLivingEntityHelper;
import me.mioclient.MixinLivingEntityHelper_2;
import me.mioclient.SearchHelper_4;
import me.mioclient.event.GetStatusEffectEvent;
import me.mioclient.event.TickPreEvent;
import me.mioclient.feature.Event_3;
import me.mioclient.module.misc.Swing;
import me.mioclient.module.movement.ElytraFly;
import me.mioclient.module.movement.FastLadder;
import me.mioclient.module.movement.Fireworks;
import me.mioclient.module.movement.Jesus;
import me.mioclient.module.movement.NoJumpDelay;
import me.mioclient.module.movement.Speed;
import net.minecraft.client.MinecraftClient;
import net.minecraft.entity.Entity;
import net.minecraft.entity.EntityType;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.damage.DamageSource;
import net.minecraft.entity.effect.StatusEffect;
import net.minecraft.entity.effect.StatusEffectInstance;
import net.minecraft.entity.effect.StatusEffectUtil;
import net.minecraft.entity.effect.StatusEffects;
import net.minecraft.registry.entry.RegistryEntry;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.World;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Constant;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.ModifyConstant;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({LivingEntity.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinLivingEntity.class */
public abstract class MixinLivingEntity extends Entity implements AutoCrystalHelper_4 {

    @Shadow
    private int field_6228;

    @Unique
    private TickPreEvent mio$moveEvent;

    @Unique
    private Event_3 event;

    @Unique
    private boolean serverDead;

    @Unique
    private boolean prevFlying;

    @Unique
    private float prevLookYaw;

    @Unique
    private float prevLookPitch;

    @Unique
    private long prevTime;
    private static ElytraFly efly = (ElytraFly) BaritoneHelper_3.baritoneHelper_4.getModule117(ElytraFly.class);
    private static NoJumpDelay njd = (NoJumpDelay) BaritoneHelper_3.baritoneHelper_4.getModule117(NoJumpDelay.class);
    private static Jesus jesus = (Jesus) BaritoneHelper_3.baritoneHelper_4.getModule117(Jesus.class);
    private static Swing swing = (Swing) BaritoneHelper_3.baritoneHelper_4.getModule117(Swing.class);
    private static final Speed speed = (Speed) BaritoneHelper_3.baritoneHelper_4.getModule117(Speed.class);
    private static final Fireworks fireworks = (Fireworks) BaritoneHelper_3.baritoneHelper_4.getModule117(Fireworks.class);
    private static final FastLadder fastladder = (FastLadder) BaritoneHelper_3.baritoneHelper_4.getModule117(FastLadder.class);

    public MixinLivingEntity(EntityType<?> entityType, World world) {
        super(entityType, world);
        this.prevTime = 0L;
    }

    @Shadow
    public abstract float getYaw(float f);

    @Shadow
    public abstract void setBodyYaw(float f);

    @Shadow
    public abstract boolean damage(DamageSource damageSource, float f);

    @Inject(method = {"getHandSwingDuration"}, at = {@At("HEAD")}, cancellable = true)
    private void getHandSwingDurationHook(CallbackInfoReturnable<Integer> callbackInfoReturnable) {
        if (swing.isToggled()) {
            LivingEntity livingEntity = (LivingEntity)(Object) this;
            callbackInfoReturnable.cancel();
            int doubleValue = (int) ((swing.type.getValue() == Swing.MixinLivingEntityMode.ONE_EIGHT ? 7 : 6) / swing.speed.getValue().doubleValue());
            if (StatusEffectUtil.hasHaste(livingEntity)) {
                callbackInfoReturnable.setReturnValue(Integer.valueOf(doubleValue - (1 + StatusEffectUtil.getHasteAmplifier(livingEntity))));
            } else {
                callbackInfoReturnable.setReturnValue(Integer.valueOf(livingEntity.hasStatusEffect(StatusEffects.MINING_FATIGUE) ? doubleValue + ((1 + livingEntity.getStatusEffect(StatusEffects.MINING_FATIGUE).getAmplifier()) * 2) : doubleValue));
            }
        }
    }

    @Inject(method = {"isFallFlying"}, at = {@At("HEAD")}, cancellable = true)
    private void isFallFlyingHook(CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        if (SearchHelper_4.is1470(this)) {
            if (fireworks.isToggled() && fireworks.fastLaunch.getValue().booleanValue() && fireworks.get143() > 0) {
                callbackInfoReturnable.cancel();
                callbackInfoReturnable.setReturnValue(true);
            }
            if (efly.is949() || efly.is956()) {
                callbackInfoReturnable.setReturnValue(false);
                callbackInfoReturnable.cancel();
            }
        }
    }

    @Inject(method = {"canWalkOnFluid"}, at = {@At("HEAD")}, cancellable = true)
    private void canWalkOnFluidHook(CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        if (((Object) this) == MinecraftClient.getInstance().player && jesus.isToggled() && jesus.is240() && !MinecraftClient.getInstance().player.isTouchingWater()) {
            callbackInfoReturnable.setReturnValue(true);
        }
    }

    @Inject(method = {"travel"}, at = {@At("HEAD")})
    private void travelPreHook(Vec3d vec3d, CallbackInfo callbackInfo) {
        if (((Object) this) != MinecraftClient.getInstance().player) {
            return;
        }
        ElytraFly.flag = true;
        this.prevLookYaw = getYaw(1.0f);
        this.prevLookPitch = getPitch();
        this.event = new Event_3(this.prevLookYaw, this.prevLookPitch, false);
        SearchHelper_4.baritoneHelper.getObject1794(this.event);
        if (this.event.is2403()) {
            setYaw(this.event.get751());
            setPitch(this.event.get752());
        }
    }

    @Inject(method = {"travel"}, at = {@At("TAIL")})
    private void travelPostHook(Vec3d vec3d, CallbackInfo callbackInfo) {
        if (((Object) this) != MinecraftClient.getInstance().player) {
            return;
        }
        ElytraFly.flag = false;
        if (this.event.is2403()) {
            setYaw(this.prevLookYaw);
            setPitch(this.prevLookPitch);
        }
    }

    @ModifyExpressionValue(method = {"jump"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/entity/LivingEntity;getYaw()F")})
    private float jumpFix(float f) {
        this.event = new Event_3(getYaw(), getPitch(), true);
        SearchHelper_4.baritoneHelper.getObject1794(this.event);
        return this.event.is2403() ? this.event.get751() : f;
    }

    @Inject(method = {"shouldSpawnConsumptionEffects"}, at = {@At("HEAD")}, cancellable = true)
    private void shouldSpawnConsumptionEffects(CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        if (MixinLivingEntityHelper.is870()) {
            callbackInfoReturnable.setReturnValue(false);
        }
    }

    @ModifyExpressionValue(method = {"damage"}, at = {@At(value = "FIELD", target = "Lnet/minecraft/world/World;isClient:Z")})
    private boolean damageHook(boolean z) {
        if (((Object) this) instanceof Feature_14.OtherClientPlayerEntity) {
            return false;
        }
        return z;
    }

    @Override // me.mioclient.AutoCrystalHelper_4
    public void setServerSideDead(boolean z) {
        this.serverDead = z;
    }

    @Override // me.mioclient.AutoCrystalHelper_4
    public boolean isServerSideDead() {
        return this.serverDead;
    }

    @Inject(method = {"tickMovement"}, at = {@At("HEAD")})
    public void reduceCooldown(CallbackInfo callbackInfo) {
        MinecraftClient minecraftClient = MinecraftClient.getInstance();
        if (efly.isToggled() && efly.mode.getValue() == ElytraFly.ElytraFlyPredicateMode.BOUNCE && equals(minecraftClient.player) && this.field_6228 > 2) {
            this.field_6228 = 2;
        } else if (njd.isToggled() && equals(minecraftClient.player) && !minecraftClient.player.isFallFlying()) {
            this.field_6228 = 0;
        }
    }

    @Inject(method = {"isFallFlying"}, at = {@At("TAIL")}, cancellable = true)
    public void onRecast(CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        if (((Object) this) != MinecraftClient.getInstance().player) {
            return;
        }
        if (efly.is956()) {
            callbackInfoReturnable.setReturnValue(false);
            return;
        }
        long currentTimeMillis = System.currentTimeMillis();
        if (currentTimeMillis - this.prevTime >= 12) {
            if (efly.isToggled() && efly.mode.getValue() == ElytraFly.ElytraFlyPredicateMode.BOUNCE) {
                boolean booleanValue = ((Boolean) callbackInfoReturnable.getReturnValue()).booleanValue();
                if (this.prevFlying && !booleanValue) {
                    if (efly.is950()) {
                        AutoSignSearchHelper4.do948();
                    }
                    callbackInfoReturnable.setReturnValue(Boolean.valueOf(efly.is950()));
                }
                this.prevFlying = booleanValue;
            }
            this.prevTime = currentTimeMillis;
        }
    }

    @Inject(method = {"hasStatusEffect"}, at = {@At("HEAD")}, cancellable = true)
    private void hasStatusEffectHook(RegistryEntry<StatusEffect> registryEntry, CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        if (equals(MinecraftClient.getInstance().player)) {
            GetStatusEffectEvent getStatusEffectEvent = new GetStatusEffectEvent(registryEntry);
            SearchHelper_4.baritoneHelper.getObject1794(getStatusEffectEvent);
            if (getStatusEffectEvent.is2403()) {
                callbackInfoReturnable.cancel();
                callbackInfoReturnable.setReturnValue(false);
            }
        }
    }

    @Inject(method = {"getStatusEffect"}, at = {@At("HEAD")}, cancellable = true)
    private void getStatusEffectHook(RegistryEntry<StatusEffect> registryEntry, CallbackInfoReturnable<StatusEffectInstance> callbackInfoReturnable) {
        if (equals(MinecraftClient.getInstance().player)) {
            GetStatusEffectEvent getStatusEffectEvent = new GetStatusEffectEvent(registryEntry);
            SearchHelper_4.baritoneHelper.getObject1794(getStatusEffectEvent);
            if (getStatusEffectEvent.is2403()) {
                callbackInfoReturnable.cancel();
                callbackInfoReturnable.setReturnValue(null);
            }
        }
    }

    @Inject(method = {"jump"}, at = {@At("HEAD")}, cancellable = true)
    private void jumpHook(CallbackInfo callbackInfo) {
        if (!speed.is130() && speed.isToggled() && speed.mode.getValue() == Speed.SpeedPredicateMode.speedPredicateMode2 && HoleSnapSearchHelper4_3.is2181()) {
            callbackInfo.cancel();
        }
    }

    @ModifyConstant(method = {"applyMovementInput"}, constant = {@Constant(doubleValue = 0.2d)})
    private double applyMovementInputHook(double d) {
        return fastladder.isToggled() ? fastladder.speed.getValue().floatValue() : d;
    }

    @Inject(method = {"applyMovementInput"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/entity/LivingEntity;move(Lnet/minecraft/entity/MovementType;Lnet/minecraft/util/math/Vec3d;)V", shift = At.Shift.BEFORE)})
    private void tickPre(Vec3d vec3d, float f, CallbackInfoReturnable<Vec3d> callbackInfoReturnable) {
        if (TickPreEvent.is1301(this)) {
            this.mio$moveEvent = new TickPreEvent(getVelocity());
            SearchHelper_4.baritoneHelper.getObject1794(this.mio$moveEvent);
            MixinLivingEntityHelper_2.do2581(getVelocity(), this.mio$moveEvent.get515(), this.mio$moveEvent.get692(), this.mio$moveEvent.get516());
        }
    }

    @Inject(method = {"travel"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/entity/LivingEntity;move(Lnet/minecraft/entity/MovementType;Lnet/minecraft/util/math/Vec3d;)V", shift = At.Shift.BEFORE)})
    private void tickPre(Vec3d vec3d, CallbackInfo callbackInfo) {
        if (TickPreEvent.is1301(this)) {
            this.mio$moveEvent = new TickPreEvent(getVelocity());
            SearchHelper_4.baritoneHelper.getObject1794(this.mio$moveEvent);
            MixinLivingEntityHelper_2.do2581(getVelocity(), this.mio$moveEvent.get515(), this.mio$moveEvent.get692(), this.mio$moveEvent.get516());
        }
    }
}
