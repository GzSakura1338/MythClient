package me.mioclient.module.combat;

import java.util.Comparator;
import java.util.Objects;
import java.util.stream.Stream;
import java.util.stream.StreamSupport;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.FireworksHelper;
import me.mioclient.FreecamHelper;
import me.mioclient.HoleSnapEvent;
import me.mioclient.MixinMessageIndicatorHelper;
import me.mioclient.PhaseESPHelper;
import me.mioclient.PhaseESPSearchHelper4_2;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.module.Module;
import net.minecraft.block.BlockState;
import net.minecraft.client.network.ClientPlayerEntity;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Items;
import net.minecraft.text.Text;
import net.minecraft.util.Hand;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Direction;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/combat/SelfWeb.class */
public class SelfWeb extends Module {
    public Setting<Boolean> rotate;
    public Setting<Boolean> autoDisable;
    public Setting<Boolean> onlyHole;
    public Setting<Boolean> smart;
    public Setting<Float> enemyRange;
    public Setting<Boolean> aggressive;

    public SelfWeb() {
        super("SelfWeb", "Places a cobweb at your feet.", Category.COMBAT, new String[0]);
        PhaseESPHelper.do1351(this);
    }

    @Listen
    public void onEvent(HoleSnapEvent holeSnapEvent) {
        Entity entity1864 = getEntity1864(this.enemyRange);
        if (!this.smart.getValue().booleanValue()) {
            do1863(holeSnapEvent);
        } else if (entity1864 != null) {
            do1863(holeSnapEvent);
        }
    }

    public void do1863(HoleSnapEvent holeSnapEvent) {
        if (minecraftClient.player.hasVehicle()) {
            return;
        }
        if (BaritoneHelper_3.holeSnapSearchHelper4_5.is2728() || !this.onlyHole.getValue().booleanValue()) {
            BlockPos ofFloored = BlockPos.ofFloored(minecraftClient.player.getPos());
            BlockState blockState = minecraftClient.world.getBlockState(ofFloored);
            if (blockState.isReplaceable() && !blockState.isLiquid() && minecraftClient.player.isOnGround()) {
                int i = FireworksHelper.get447(Items.COBWEB);
                int i2 = minecraftClient.player.getInventory().selectedSlot;
                if (i == -1) {
                    MixinMessageIndicatorHelper.do345(Text.literal(getName()).append(" is out of blocks!"), MixinMessageIndicatorHelper.getMessageSignatureData337(-2), MixinMessageIndicatorHelper.MixinClientConnectionMode.mixinClientConnectionMode);
                    disable();
                    return;
                }
                Direction direction3028 = PhaseESPSearchHelper4_2.getDirection3028(ofFloored);
                if (direction3028 == null) {
                    return;
                }
                FireworksHelper.do456(i);
                Box boundingBox = minecraftClient.player.getBoundingBox();
                minecraftClient.player.setBoundingBox(new Box(0.0d, 0.0d, 0.0d, 0.0d, 0.0d, 0.0d));
                PhaseESPSearchHelper4_2.is3037(ofFloored, direction3028, false, Hand.MAIN_HAND);
                minecraftClient.player.setBoundingBox(boundingBox);
                FireworksHelper.do456(i2);
                if (this.rotate.getValue().booleanValue()) {
                    BaritoneHelper_3.searchHelper4_8.do2477(new float[]{holeSnapEvent.get751(), FreecamHelper.num2}, 5);
                }
                if (this.autoDisable.getValue().booleanValue()) {
                    disable();
                }
            }
        }
    }

    public Entity getEntity1864(Setting<Float> setting) {
        Stream<Entity> filter = StreamSupport.stream(minecraftClient.world.getEntities().spliterator(), false).filter(entity -> {
            if (entity != minecraftClient.player && minecraftClient.player.distanceTo(entity) <= ((Float) setting.getValue()).floatValue() && is1865(entity) && (entity instanceof PlayerEntity)) {
                if (!BaritoneHelper_3.searchHelper4_14.is519(entity.getName().getString()) && entity.isAlive()) {
                    return true;
                }
            }
            return false;
        });
        ClientPlayerEntity clientPlayerEntity = minecraftClient.player;
        Objects.requireNonNull(clientPlayerEntity);
        return (Entity) filter.min(Comparator.comparing(clientPlayerEntity::squaredDistanceTo)).orElse(null);
    }

    public boolean is1865(Entity entity) {
        return this.aggressive.getValue().booleanValue() ? entity.getY() >= minecraftClient.player.getY() : entity.getY() > minecraftClient.player.getY();
    }
}
