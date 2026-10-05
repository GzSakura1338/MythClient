package me.mioclient.module.combat;

import java.util.Iterator;
import me.mioclient.AutoCrystalHelper_3;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.FreecamHelper;
import me.mioclient.KeyPearlMode;
import me.mioclient.PhaseESPHelper;
import me.mioclient.SearchHelper4_7;
import me.mioclient.SearchHelper4_8;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.InteractItemEvent_2;
import me.mioclient.event.Listen;
import me.mioclient.feature.MotionEvent;
import me.mioclient.module.Module;
import net.minecraft.client.network.ClientPlayerEntity;
import net.minecraft.entity.Entity;
import net.minecraft.entity.effect.StatusEffect;
import net.minecraft.entity.effect.StatusEffectInstance;
import net.minecraft.entity.effect.StatusEffects;
import net.minecraft.entity.mob.EndermanEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.ArrowItem;
import net.minecraft.item.CrossbowItem;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;
import net.minecraft.item.RangedWeaponItem;
import net.minecraft.item.TridentItem;
import net.minecraft.network.packet.c2s.play.PlayerInteractItemC2SPacket;
import net.minecraft.potion.Potion;
import net.minecraft.registry.entry.RegistryEntry;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Vec3d;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/combat/BowAim.class */
public class BowAim extends Module {
    public Setting<Boolean> targets;
    public Setting<Boolean> players;
    public Setting<Boolean> ignoreNakeds;
    public Setting<Boolean> animals;
    public Setting<Boolean> hostiles;
    public Setting<Boolean> neutrals;
    public float[] floatArr;

    public BowAim() {
        super("BowAim", "Helps aiming with bows.", Category.COMBAT, new String[0]);
        PhaseESPHelper.do1351(this);
    }

    @Listen
    public void do31(MotionEvent motionEvent) {
        Item item = minecraftClient.player.getMainHandStack().getItem();
        boolean z = (item instanceof RangedWeaponItem) || (item instanceof TridentItem);
        boolean isUsingItem = minecraftClient.player.isUsingItem();
        if (CrossbowItem.isCharged(minecraftClient.player.getMainHandStack())) {
            isUsingItem = true;
        }
        if (z && isUsingItem && motionEvent.getKeyPearlMode1472() == KeyPearlMode.Pre) {
            Iterator it = getPotion1955().getEffects().iterator();
            while (it.hasNext()) {
                RegistryEntry effectType = ((StatusEffectInstance) it.next()).getEffectType();
                if (((StatusEffect) effectType.value()).isBeneficial() && effectType != StatusEffects.SLOW_FALLING && effectType != StatusEffects.JUMP_BOOST) {
                    return;
                }
            }
            Entity entity2440 = SearchHelper4_7.getEntity2440(this.players.getValue().booleanValue(), this.neutrals.getValue().booleanValue(), this.animals.getValue().booleanValue(), this.hostiles.getValue().booleanValue(), false, false, 0.0f, Float.intBitsToFloat(1124073472), false, this.ignoreNakeds.getValue().booleanValue());
            if (entity2440 == null || (entity2440 instanceof EndermanEntity)) {
                return;
            }
            Box boundingBox = ((Entity) entity2440).getBoundingBox();
            if (entity2440 instanceof PlayerEntity) {
                boundingBox = BaritoneHelper_3.mainhandHelper_2.getBox1109((PlayerEntity) entity2440, 5);
            }
            Vec3d center = boundingBox.getCenter();
            if (SearchHelper4_8.is2492(boundingBox) && SearchHelper4_7.is2433(center)) {
                float[] floatArray1956 = getFloatArray1956((Entity) entity2440, true, Float.intBitsToFloat(1073741824));
                if (Float.isNaN(floatArray1956[0]) || Float.isNaN(floatArray1956[1])) {
                    return;
                }
                this.floatArr = floatArray1956;
                BaritoneHelper_3.searchHelper4_8.do2478(floatArray1956, 5, true);
            }
        }
    }

    @Listen
    public void onInteractItem(InteractItemEvent_2 interactItemEvent_2) {
        PlayerInteractItemC2SPacket playerInteractItemC2SPacket1816 = interactItemEvent_2.getPlayerInteractItemC2SPacket1816();
        if (!minecraftClient.player.getStackInHand(playerInteractItemC2SPacket1816.getHand()).isOf(Items.CROSSBOW) || this.floatArr == null) {
            return;
        }
        interactItemEvent_2.do1818(this.floatArr);
    }

    public Potion getPotion1955() {
        for (int i = 0; i < minecraftClient.player.getInventory().size(); i++) {
            ItemStack stack = minecraftClient.player.getInventory().getStack(i);
            if (stack.getItem() instanceof ArrowItem) {
                return AutoCrystalHelper_3.getPotion1565(stack);
            }
        }
        return AutoCrystalHelper_3.potion;
    }

    public static float[] getFloatArray1956(Entity entity, boolean z, float f) {
        ClientPlayerEntity clientPlayerEntity = minecraftClient.player;
        double x = (entity.getX() + (z ? (entity.getX() - entity.prevX) * f : 0.0d)) - (clientPlayerEntity.getX() + (z ? clientPlayerEntity.getX() - clientPlayerEntity.prevX : 0.0d));
        double eyeHeight = ((((entity.getBoundingBox().minY + (z ? (entity.getBoundingBox().minY - entity.prevY) * f : 0.0d)) + entity.getEyeHeight(entity.getPose())) - Double.longBitsToDouble(4594572339843380019L)) - (clientPlayerEntity.getBoundingBox().minY + (z ? clientPlayerEntity.getY() - clientPlayerEntity.prevY : 0.0d))) - clientPlayerEntity.getEyeHeight(clientPlayerEntity.getPose());
        double z2 = (entity.getZ() + (z ? (entity.getZ() - entity.prevZ) * f : 0.0d)) - (clientPlayerEntity.getZ() + (z ? clientPlayerEntity.getZ() - clientPlayerEntity.prevZ : 0.0d));
        double hypot = Math.hypot(x, z2);
        float itemUseTime = clientPlayerEntity.getItemUseTime() / Float.intBitsToFloat(1101004800);
        if (clientPlayerEntity.isHolding(Items.CROSSBOW)) {
            itemUseTime = Float.intBitsToFloat(1065353216);
        }
        float intBitsToFloat = ((itemUseTime * itemUseTime) + (itemUseTime * Float.intBitsToFloat(1073741824))) / Float.intBitsToFloat(1077936128);
        if (intBitsToFloat > Float.intBitsToFloat(1065353216)) {
            intBitsToFloat = Float.intBitsToFloat(1065353216);
        }
        return new float[]{((float) ((Math.atan2(z2, x) * Double.longBitsToDouble(4640537203540230144L)) / FreecamHelper.val)) - FreecamHelper.num2, (float) (-Math.toDegrees(Math.atan(((intBitsToFloat * intBitsToFloat) - Math.sqrt((((intBitsToFloat * intBitsToFloat) * intBitsToFloat) * intBitsToFloat) - (Double.longBitsToDouble(4573567551241453568L) * ((Double.longBitsToDouble(4573567551241453568L) * (hypot * hypot)) + ((Double.longBitsToDouble(4611686018427387904L) * eyeHeight) * (intBitsToFloat * intBitsToFloat)))))) / (Double.longBitsToDouble(4573567551241453568L) * hypot))))};
    }
}
