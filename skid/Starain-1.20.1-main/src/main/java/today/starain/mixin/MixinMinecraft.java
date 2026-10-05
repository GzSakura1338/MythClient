package today.starain.mixin;

import com.mojang.blaze3d.platform.Window;
import net.minecraft.client.Minecraft;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import today.starain.Bootstrap;
import today.starain.Starain;
import today.starain.event.api.EventType;
import today.starain.event.impl.ClickEvent;
import today.starain.event.impl.GameTickEvent;
import today.starain.utils.animation.AnimationUtils;
import today.starain.utils.render.shader.impl.KawaseBlur;
import today.starain.utils.render.skia.context.SkiaContext;

// 从奶粉摩登大话来的。
@Mixin({Minecraft.class})
public class MixinMinecraft {

    @Unique
    private long lastFrame;

    @Inject(
            method = {"<init>"},
            at = {@At("TAIL")}
    )
    private void onInit(CallbackInfo info) {
        // 我的世界最强反破解
        new Bootstrap("破解全家死光亲妈猪逼被操烂亲爹没鸡巴生小孩没屁眼操你血妈");
    }

    @Inject(
            method = {"tick"},
            at = {@At("HEAD")}
    )
    private void tickPre(CallbackInfo ci) {
        if (Starain.getInstance() != null && Starain.getInstance().getEventManager() != null) {
            Starain.getInstance().getEventManager().call(new GameTickEvent(EventType.PRE));
        }
    }

    @Inject(
            method = {"tick"},
            at = {@At("TAIL")}
    )
    private void tickPost(CallbackInfo ci) {
        if (Starain.getInstance() != null && Starain.getInstance().getEventManager() != null) {
            Starain.getInstance().getEventManager().call(new GameTickEvent(EventType.POST));
        }
    }

    @Inject(
            method = {"handleKeybinds"},
            at = {@At(
                    value = "INVOKE",
                    target = "Lnet/minecraft/client/player/LocalPlayer;isUsingItem()Z",
                    ordinal = 0,
                    shift = At.Shift.BEFORE
            )},
            cancellable = true
    )
    private void clickEvent(CallbackInfo ci) {
        if (Starain.getInstance() != null && Starain.getInstance().getEventManager() != null) {
            ClickEvent event = new ClickEvent();
            Starain.getInstance().getEventManager().call(event);
            if (event.isCancelled()) {
                ci.cancel();
            }
        }
    }

    @Inject(
            method = {"runTick"},
            at = {@At("HEAD")}
    )
    private void runTick(CallbackInfo ci) {
        long currentTime = System.nanoTime() / 1000000L;
        int deltaTime = (int) (currentTime - this.lastFrame);
        this.lastFrame = currentTime;
        AnimationUtils.delta = deltaTime;
    }

    @Inject(
            method = {"resizeDisplay"},
            at = {@At("HEAD")}
    )
    private void onResizeDisplay(CallbackInfo ci) {
        Window window = Minecraft.getInstance().getWindow();
        SkiaContext.createSurface(window.getWidth(), window.getHeight());
        KawaseBlur.INSTANCE.resize();
    }
}
