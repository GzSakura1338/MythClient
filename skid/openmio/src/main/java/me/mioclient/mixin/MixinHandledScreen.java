package me.mioclient.mixin;

import com.llamalad7.mixinextras.injector.v2.WrapWithCondition;
import com.llamalad7.mixinextras.sugar.Local;
import java.util.List;
import java.util.Optional;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.EntityControlSearchHelper4;
import me.mioclient.FireworksHelper;
import me.mioclient.HeightSearchHelper4;
import me.mioclient.MixinDrawContextHelper;
import me.mioclient.MixinTextFieldWidgetHelper;
import me.mioclient.SearchHelper_4;
import me.mioclient.event.RenderWithTooltipEvent;
import me.mioclient.feature.Event_2;
import me.mioclient.mixin.ducks.DuckDrawContext;
import me.mioclient.module.misc.ChestSearchBar;
import me.mioclient.module.render.Tooltips;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.screen.ingame.HandledScreen;
import net.minecraft.client.gui.tooltip.HoveredTooltipPositioner;
import net.minecraft.component.DataComponentTypes;
import net.minecraft.component.type.MapIdComponent;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;
import net.minecraft.item.map.MapState;
import net.minecraft.item.tooltip.TooltipData;
import net.minecraft.screen.slot.Slot;
import net.minecraft.text.Text;
import org.jetbrains.annotations.Nullable;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({HandledScreen.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinHandledScreen.class */
public class MixinHandledScreen implements SearchHelper_4 {
    private static final ChestSearchBar chestsearchbar = (ChestSearchBar) BaritoneHelper_3.baritoneHelper_4.getModule117(ChestSearchBar.class);
    private static final Tooltips tooltips = (Tooltips) BaritoneHelper_3.baritoneHelper_4.getModule117(Tooltips.class);

    @Shadow
    @Nullable
    protected Slot field_2787;

    @Unique
    private int lastMX;

    @Unique
    private int lastMY;

    @Unique
    private int mio$lastTooltipX;

    @Unique
    private int mio$lastTooltipY;

    @Unique
    private Slot mio$lastSlot;

    @Unique
    private MapState mio$cachedState;

    @Unique
    private int mio$cachedId;

    @Unique
    private final Event_2 event = new Event_2((HandledScreen)(Object) this);

    @Inject(method = {"keyPressed"}, at = {@At("HEAD")}, cancellable = true)
    public void keyPressed(int i, int i2, int i3, CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        if (chestsearchbar != null && chestsearchbar.isToggled() && MixinTextFieldWidgetHelper.textFieldWidget != null && MixinTextFieldWidgetHelper.textFieldWidget.isFocused() && minecraftClient.options.inventoryKey.matchesKey(i, i2)) {
            callbackInfoReturnable.setReturnValue(true);
        }
    }

    @Inject(method = {"tick"}, at = {@At("HEAD")})
    private void tick(CallbackInfo callbackInfo) {
        this.event.do1457();
    }

    @Inject(method = {"drawMouseoverTooltip"}, at = {@At("HEAD")})
    private void drawMouseoverTooltipHook(DrawContext drawContext, int i, int i2, CallbackInfo callbackInfo) {
        this.event.do2379(drawContext);
        this.event.setX(i);
        this.event.setY(i2);
        baritoneHelper.getObject1794(this.event);
    }

    @Inject(method = {"render"}, at = {@At("HEAD")})
    private void render(DrawContext drawContext, int i, int i2, float f, CallbackInfo callbackInfo) {
        if (!EntityControlSearchHelper4.is2605(342)) {
            this.mio$lastSlot = this.field_2787;
        }
        this.lastMX = i;
        this.lastMY = i2;
    }

    @Inject(method = {"drawMouseoverTooltip"}, at = {@At("HEAD")})
    private void renderWithTooltipHook(DrawContext drawContext, int i, int i2, CallbackInfo callbackInfo) {
        if (tooltips.isToggled() && this.mio$lastSlot != null && FireworksHelper.is135(this.mio$lastSlot.getStack().getItem()) && EntityControlSearchHelper4.is2605(342)) {
            this.field_2787 = this.mio$lastSlot;
        }
    }

    @Redirect(method = {"drawMouseoverTooltip"}, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/gui/DrawContext;drawTooltip(Lnet/minecraft/client/font/TextRenderer;Ljava/util/List;Ljava/util/Optional;II)V"))
    private void renderWithTooltip(DrawContext drawContext, TextRenderer textRenderer, List<Text> list, Optional<TooltipData> optional, int i, int i2) {
        if ((!EntityControlSearchHelper4.is2605(342)) || this.mio$lastSlot == null || !FireworksHelper.is135(this.mio$lastSlot.getStack().getItem())) {
            this.mio$lastTooltipX = i;
            this.mio$lastTooltipY = i2;
        } else {
            i = this.mio$lastTooltipX;
            i2 = this.mio$lastTooltipY;
        }
        RenderWithTooltipEvent renderWithTooltipEvent = new RenderWithTooltipEvent((Screen)(Object) this, MixinDrawContextHelper.getList1128(drawContext, list, optional), i, i2, this.lastMX, this.lastMY);
        SearchHelper_4.baritoneHelper.getObject1794(renderWithTooltipEvent);
        if (renderWithTooltipEvent.is2403()) {
            return;
        }
        ((DuckDrawContext) drawContext).drawTooltipsHook(textRenderer, renderWithTooltipEvent.getList248(), i, i2, HoveredTooltipPositioner.INSTANCE);
    }

    @Inject(method = {"drawSlot"}, at = {@At("HEAD")})
    private void drawSlot(DrawContext drawContext, Slot slot, CallbackInfo callbackInfo) {
        int id;
        this.mio$cachedState = null;
        ItemStack stack = slot.getStack();
        if (stack.isOf(Items.FILLED_MAP) && (id = ((MapIdComponent) stack.getOrDefault(DataComponentTypes.MAP_ID, new MapIdComponent(-1))).id()) != -1) {
            this.mio$cachedId = id;
            this.mio$cachedState = BaritoneHelper_3.tooltipsSearchHelper4.getMapState2653(stack, id);
        }
    }

    @WrapWithCondition(method = {"drawSlot"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/gui/DrawContext;drawItem(Lnet/minecraft/item/ItemStack;III)V")})
    private boolean drawSlotHook(DrawContext drawContext, ItemStack itemStack, int i, int i2, int i3) {
        return (tooltips.isToggled() && tooltips.mapOverlay.getValue().booleanValue() && this.mio$cachedState != null) ? false : true;
    }

    @WrapWithCondition(method = {"drawSlot"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/gui/DrawContext;drawItemInSlot(Lnet/minecraft/client/font/TextRenderer;Lnet/minecraft/item/ItemStack;IILjava/lang/String;)V")})
    private boolean drawSlotHook(DrawContext drawContext, TextRenderer textRenderer, ItemStack itemStack, int i, int i2, String str, @Local(argsOnly = true) Slot slot) {
        if (!tooltips.isToggled() || !tooltips.mapOverlay.getValue().booleanValue() || this.mio$cachedState == null) {
            return true;
        }
        HeightSearchHelper4.do2376(drawContext.getMatrices(), this.mio$cachedState, this.mio$cachedId, i, i2);
        if (EntityControlSearchHelper4.is2605(342)) {
            return false;
        }
        drawContext.drawItemInSlot(textRenderer, itemStack, i, i2);
        return false;
    }
}
