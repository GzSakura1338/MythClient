package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import com.llamalad7.mixinextras.injector.WrapWithCondition;
import com.llamalad7.mixinextras.sugar.Local;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.FontsEvent;
import me.mioclient.FontsSearchHelper4_2;
import me.mioclient.SearchHelper_4;
import me.mioclient.event.DisconnectEvent;
import me.mioclient.event.RenderEvent;
import me.mioclient.event.SetScreenHookPostEvent;
import me.mioclient.event.SetScreenHookPreEvent;
import me.mioclient.feature.ConfirmGameClose;
import me.mioclient.module.client.UI;
import me.mioclient.module.combat.Arrows;
import me.mioclient.module.combat.AutoClicker;
import me.mioclient.module.combat.AutoCrystal;
import me.mioclient.module.combat.NoHitDelay;
import me.mioclient.module.exploit.MultiTask;
import me.mioclient.module.misc.AntiQuit;
import me.mioclient.module.misc.UnfocusedFPS;
import me.mioclient.module.player.AutoEat;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.network.ClientPlayerInteractionManager;
import net.minecraft.client.option.GameOptions;
import net.minecraft.client.util.Window;
import org.jetbrains.annotations.Nullable;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({MinecraftClient.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinMinecraft.class */
public class MixinMinecraft {

    @Unique
    private Screen mio$lastScreen;

    @Shadow
    private boolean field_1695;

    @Shadow
    public int field_1771;

    @Shadow
    @Final
    public GameOptions field_1690;

    @Shadow
    @Nullable
    public Screen field_1755;

    @Shadow
    @Final
    private Window field_1704;
    private static UI clickgui = (UI) BaritoneHelper_3.baritoneHelper_4.getModule117(UI.class);
    private static MultiTask multitask = (MultiTask) BaritoneHelper_3.baritoneHelper_4.getModule117(MultiTask.class);
    private static AutoEat autoeat = (AutoEat) BaritoneHelper_3.baritoneHelper_4.getModule117(AutoEat.class);
    private static UnfocusedFPS unfocusedfps = (UnfocusedFPS) BaritoneHelper_3.baritoneHelper_4.getModule117(UnfocusedFPS.class);
    private static AntiQuit antiquit = (AntiQuit) BaritoneHelper_3.baritoneHelper_4.getModule117(AntiQuit.class);
    private static NoHitDelay nohitdelay = (NoHitDelay) BaritoneHelper_3.baritoneHelper_4.getModule117(NoHitDelay.class);
    private static AutoClicker autoclicker = (AutoClicker) BaritoneHelper_3.baritoneHelper_4.getModule117(AutoClicker.class);
    private static final Arrows arrows = (Arrows) BaritoneHelper_3.baritoneHelper_4.getModule117(Arrows.class);
    private static final AutoCrystal autocrystal = (AutoCrystal) BaritoneHelper_3.baritoneHelper_4.getModule117(AutoCrystal.class);
    private static boolean saved = false;

    @Inject(method = {"doAttack()Z"}, at = {@At("HEAD")})
    public void doAttackHook(CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        if (nohitdelay.isToggled() || autoclicker.isToggled()) {
            this.field_1771 = 0;
        }
    }

    @Inject(method = {"stop"}, at = {@At("HEAD")})
    public void scheduleStop(CallbackInfo callbackInfo) {
        if (saved || BaritoneHelper_3.presetHelper == null) {
            return;
        }
        saved = true;
        clickgui.disable();
        BaritoneHelper_3.presetHelper.do41();
    }

    @Inject(method = {"disconnect(Lnet/minecraft/client/gui/screen/Screen;)V"}, at = {@At("HEAD")})
    public void disconnect(Screen screen, CallbackInfo callbackInfo) {
        SearchHelper_4.baritoneHelper.getObject1794(new DisconnectEvent());
    }

    @ModifyExpressionValue(method = {"handleBlockBreaking"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/network/ClientPlayerEntity;isUsingItem()Z")})
    public boolean handleBlockBreakingHook(boolean z) {
        return !multitask.isToggled() && z;
    }

    @Redirect(method = {"doItemUse"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/network/ClientPlayerInteractionManager;isBreakingBlock()Z"), require = 0)
    public boolean doItemUseHook(ClientPlayerInteractionManager clientPlayerInteractionManager) {
        return !multitask.isToggled() && clientPlayerInteractionManager.isBreakingBlock();
    }

    @Inject(method = {"setScreen"}, at = {@At("HEAD")}, cancellable = true)
    private void setScreenHookPre(Screen screen, CallbackInfo callbackInfo) {
        this.mio$lastScreen = this.field_1755;
        SetScreenHookPreEvent setScreenHookPreEvent = new SetScreenHookPreEvent(this.mio$lastScreen, screen);
        SearchHelper_4.baritoneHelper.getObject1794(setScreenHookPreEvent);
        if (setScreenHookPreEvent.is2403()) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"setScreen"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/gui/screen/Screen;init(Lnet/minecraft/client/MinecraftClient;II)V", shift = At.Shift.AFTER)}, cancellable = true)
    private void setScreenHookPost(Screen screen, CallbackInfo callbackInfo) {
        SetScreenHookPostEvent setScreenHookPostEvent = new SetScreenHookPostEvent(this.mio$lastScreen, screen);
        SearchHelper_4.baritoneHelper.getObject1794(setScreenHookPostEvent);
        if (setScreenHookPostEvent.is2403()) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"tick"}, at = {@At("HEAD")})
    private void tickHook(CallbackInfo callbackInfo) {
        SearchHelper_4.baritoneHelper.getObject1794(FontsEvent.fontsEvent);
    }

    @ModifyExpressionValue(method = {"handleInputEvents"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/network/ClientPlayerEntity;isUsingItem()Z")})
    private boolean handleInputEventsHook(boolean z) {
        if (autoeat.is2086() && autoeat.isToggled()) {
            return false;
        }
        return z;
    }

    @Inject(method = {"getFramerateLimit"}, at = {@At("HEAD")}, cancellable = true)
    public void getFramerateLimit(CallbackInfoReturnable<Integer> callbackInfoReturnable) {
        if (!unfocusedfps.isToggled() || this.field_1695) {
            if ((this.field_1755 instanceof FontsSearchHelper4_2) && this.field_1695) {
                callbackInfoReturnable.setReturnValue(Integer.valueOf(this.field_1704.getFramerateLimit()));
                return;
            }
            return;
        }
        if (unfocusedfps.fps.is2348()) {
            callbackInfoReturnable.setReturnValue(Integer.valueOf(unfocusedfps.get2003()));
        } else {
            callbackInfoReturnable.setReturnValue(unfocusedfps.fps.getValue());
        }
    }

    @WrapWithCondition(method = {"render"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/MinecraftClient;scheduleStop()V")})
    private boolean renderHook(MinecraftClient minecraftClient) {
        if (!antiquit.isToggled() || !antiquit.gameClose.getValue().booleanValue()) {
            return true;
        }
        if (minecraftClient.currentScreen instanceof ConfirmGameClose) {
            return false;
        }
        minecraftClient.setScreen(new ConfirmGameClose());
        return false;
    }

    @Inject(method = {"render"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/MinecraftClient;runTasks()V", shift = At.Shift.AFTER)})
    private void render(boolean z, CallbackInfo callbackInfo, @Local int i) {
        SearchHelper_4.baritoneHelper.getObject1794(new RenderEvent(i >= 1));
    }

    @ModifyExpressionValue(method = {"handleInputEvents"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/network/ClientPlayerEntity;isUsingItem()Z")})
    private boolean handleInputEvents(boolean z) {
        if (arrows.isToggled() && arrows.autoShoot.getValue().booleanValue()) {
            return false;
        }
        return z;
    }

    @Inject(method = {"doItemUse"}, at = {@At("HEAD")})
    private void doItemUse(CallbackInfo callbackInfo) {
        autocrystal.do1162();
    }

    @ModifyExpressionValue(method = {"doItemUse"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/network/ClientPlayerEntity;isRiding()Z")})
    private boolean doItemUseHook(boolean z) {
        if (multitask.isToggled()) {
            return false;
        }
        return z;
    }

    @ModifyExpressionValue(method = {"doAttack"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/network/ClientPlayerEntity;isRiding()Z")})
    private boolean doAttack(boolean z) {
        if (multitask.isToggled()) {
            return false;
        }
        return z;
    }
}
