package today.starain.mixin;

import com.mojang.authlib.GameProfile;
import net.minecraft.client.Minecraft;
import net.minecraft.client.multiplayer.ClientLevel;
import net.minecraft.client.multiplayer.ClientPacketListener;
import net.minecraft.client.player.AbstractClientPlayer;
import net.minecraft.client.player.LocalPlayer;
import net.minecraft.network.protocol.game.ServerboundMovePlayerPacket;
import net.minecraft.network.protocol.game.ServerboundPlayerCommandPacket;
import net.minecraft.util.Mth;
import net.minecraft.world.phys.Vec3;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import today.starain.Starain;
import today.starain.event.api.EventType;
import today.starain.event.impl.MotionEvent;
import today.starain.event.impl.PlayerTickEvent;
import today.starain.event.impl.RespawnEvent;
import today.starain.event.impl.SlowdownEvent;

@Mixin({LocalPlayer.class})
public abstract class MixinLocalPlayer extends AbstractClientPlayer {
    @Shadow
    @Final
    public ClientPacketListener connection;
    @Shadow
    @Final
    protected Minecraft minecraft;
    @Shadow
    private boolean wasSprinting;
    @Shadow
    private boolean wasShiftKeyDown;
    @Shadow
    private double xLast;
    @Shadow
    private double yLast1;
    @Shadow
    private double zLast;
    @Shadow
    private float yRotLast;
    @Shadow
    private float xRotLast;
    @Shadow
    private int positionReminder;
    @Shadow
    private boolean lastOnGround;
    @Shadow
    private boolean autoJumpEnabled;

    public MixinLocalPlayer(ClientLevel pClientLevel, GameProfile pGameProfile) {
        super(pClientLevel, pGameProfile);
    }

    @Shadow
    protected abstract boolean isControlledCamera();

    @Shadow
    protected abstract void sendIsSprintingIfNeeded();

    @Inject(
            method = {"tick"},
            at = {@At(
                    value = "INVOKE",
                    target = "Lnet/minecraft/client/player/AbstractClientPlayer;tick()V",
                    shift = At.Shift.BEFORE
            )},
            cancellable = true
    )
    public void injectUpdateEvent(CallbackInfo ci) {
        PlayerTickEvent event = new PlayerTickEvent();
        Starain.getInstance().getEventManager().call(event);
        if (event.isCancelled()) {
            ci.cancel();
        }
    }

    /**
     * 原版 sendPosition 整个不跑，改由这里重写一遍，好让 MotionEvent 能改写发出去的位置和视角包。
     *
     * <p>这里以前是 @Overwrite。@Overwrite 会把方法体合并进 LocalPlayer，sendPosition 从此归本 mixin 所有，
     * 别的 mod 再想往里注入，mixin 会要求它的优先级严格高于合并方——同为默认的 1000 就直接抛
     * InvalidInjectionException 崩游戏（controlify 挂在 options.autoJump().get() 上的 autoJump 补丁就是这么炸的）。
     * 换成 HEAD + cancel 后，原版方法体原封不动留在类里（只是永远执行不到），别人的注入点照样找得到，互不打架。
     */
    @Inject(method = "sendPosition", at = @At("HEAD"), cancellable = true)
    private void onSendPosition(CallbackInfo ci) {
        ci.cancel();

        MotionEvent eventPre = new MotionEvent(EventType.PRE, this.getX(), this.getY(), this.getZ(), this.getYRot(), this.getXRot(), this.onGround());
        Starain.getInstance().getEventManager().call(eventPre);
        if (this.tickCount <= 1) {
            Starain.getInstance().getEventManager().call(new RespawnEvent());
        }
        if (eventPre.isCancelled()) {
            Starain.getInstance().getEventManager().call(new MotionEvent(EventType.POST, eventPre.getYaw(), eventPre.getPitch()));
        } else {
            this.sendIsSprintingIfNeeded();
            boolean flag3 = this.isShiftKeyDown();
            if (flag3 != this.wasShiftKeyDown) {
                ServerboundPlayerCommandPacket.Action serverboundplayercommandpacket$action1 = flag3 ? ServerboundPlayerCommandPacket.Action.PRESS_SHIFT_KEY : ServerboundPlayerCommandPacket.Action.RELEASE_SHIFT_KEY;
                this.connection.send(new ServerboundPlayerCommandPacket(this, serverboundplayercommandpacket$action1));
                this.wasShiftKeyDown = flag3;
            }

            if (this.isControlledCamera()) {
                double d4 = eventPre.getX() - this.xLast;
                double d0 = eventPre.getY() - this.yLast1;
                double d1 = eventPre.getZ() - this.zLast;
                double d2 = eventPre.getYaw() - this.yRotLast;
                double d3 = eventPre.getPitch() - this.xRotLast;
                this.positionReminder++;
                boolean flag1 = Mth.lengthSquared(d4, d0, d1) > Mth.square(2.0E-4) || this.positionReminder >= 20;
                boolean flag2 = d2 != 0.0 || d3 != 0.0;
                if (this.isPassenger()) {
                    Vec3 vec3 = this.getDeltaMovement();
                    this.connection.send(new ServerboundMovePlayerPacket.PosRot(vec3.x, -999.0, vec3.z, eventPre.getYaw(), eventPre.getPitch(), eventPre.isOnGround()));
                    flag1 = false;
                } else if (flag1 && flag2) {
                    this.connection
                            .send(new ServerboundMovePlayerPacket.PosRot(eventPre.getX(), eventPre.getY(), eventPre.getZ(), eventPre.getYaw(), eventPre.getPitch(), eventPre.isOnGround()));
                } else if (flag1) {
                    this.connection.send(new ServerboundMovePlayerPacket.Pos(eventPre.getX(), eventPre.getY(), eventPre.getZ(), eventPre.isOnGround()));
                } else if (flag2) {
                    this.connection.send(new ServerboundMovePlayerPacket.Rot(eventPre.getYaw(), eventPre.getPitch(), eventPre.isOnGround()));
                } else if (this.lastOnGround != eventPre.isOnGround()) {
                    this.connection.send(new ServerboundMovePlayerPacket.StatusOnly(eventPre.isOnGround()));
                }

                if (flag1) {
                    this.xLast = eventPre.getX();
                    this.yLast1 = eventPre.getY();
                    this.zLast = eventPre.getZ();
                    this.positionReminder = 0;
                }

                if (flag2) {
                    this.yRotLast = eventPre.getYaw();
                    this.xRotLast = eventPre.getPitch();
                }

                this.lastOnGround = eventPre.isOnGround();
                this.autoJumpEnabled = this.minecraft.options.autoJump().get();
            }

            Starain.getInstance().getEventManager().call(new MotionEvent(EventType.POST, eventPre.getYaw(), eventPre.getPitch()));
        }
    }

    @Redirect(
            method = {"aiStep"},
            at = @At(
                    value = "INVOKE",
                    target = "Lnet/minecraft/client/player/LocalPlayer;isUsingItem()Z",
                    ordinal = 0
            )
    )
    public boolean onSlowdown(LocalPlayer localPlayer) {
        if (localPlayer.isUsingItem()) {
            SlowdownEvent event = new SlowdownEvent();
            Starain.getInstance().getEventManager().call(event);
            return !event.isCancelled();
        }
        return false;
    }
}
