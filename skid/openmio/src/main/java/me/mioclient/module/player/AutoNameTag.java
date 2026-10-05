package me.mioclient.module.player;

import java.util.Set;
import java.util.concurrent.TimeUnit;
import me.mioclient.AutoCrystalMode_2;
import me.mioclient.FireworksHelper;
import me.mioclient.KeyPearlMode;
import me.mioclient.PhaseESPHelper;
import me.mioclient.PhaseESPSearchHelper4_2;
import me.mioclient.ScaffoldMode_2;
import me.mioclient.SearchHelper4_8;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.event.Listen;
import me.mioclient.feature.MotionEvent;
import me.mioclient.feature.Stopwatch;
import me.mioclient.module.Module;
import net.minecraft.component.DataComponentTypes;
import net.minecraft.entity.Entity;
import net.minecraft.entity.EntityType;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;
import net.minecraft.util.Hand;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/player/AutoNameTag.class */
public class AutoNameTag extends Module {
    public Setting<ScaffoldMode_2> mode;
    public Setting<Set<EntityType<?>>> whitelist;
    public Setting<Float> range;
    public Setting<Integer> frequency;
    public Setting<Float> delay;
    public Setting<Boolean> rotate;
    public Setting<AutoCrystalMode_2> swap;
    public final Stopwatch stopwatch;

    public AutoNameTag() {
        super("AutoNameTag", "Will use name tags on nearby entities.", Category.PLAYER, new String[0]);
        PhaseESPHelper.do1351(this);
        this.stopwatch = new Stopwatch();
    }

    @Listen
    public void do31(MotionEvent motionEvent) {
        if (motionEvent.getKeyPearlMode1472() == KeyPearlMode.Post) {
            return;
        }
        if (this.stopwatch.is418(this.delay.getValue().floatValue(), TimeUnit.SECONDS)) {
            int i = FireworksHelper.get448(this::is2998);
            int i2 = minecraftClient.player.getInventory().selectedSlot;
            int i3 = 0;
            if (this.swap.getValue() != AutoCrystalMode_2.NONE) {
                FireworksHelper.do456(i);
            }
            for (Entity entity : minecraftClient.world.getEntities()) {
                if (!is2998(minecraftClient.player.getMainHandStack())) {
                    break;
                }
                if (this.mode.getValue().is1392(entity.getType(), this.whitelist) && !(entity instanceof PlayerEntity)) {
                    if (!minecraftClient.player.getMainHandStack().getName().getString().equalsIgnoreCase(entity.hasCustomName() ? entity.getCustomName().getString() : "")) {
                        if (minecraftClient.player.getEyePos().distanceTo(entity.getPos()) <= this.range.getValue().floatValue()) {
                            PhaseESPSearchHelper4_2.do3049(entity, Hand.MAIN_HAND);
                            i3++;
                            if (this.rotate.getValue().booleanValue()) {
                                motionEvent.do2257(SearchHelper4_8.getFloatArray2484(entity.getPos()));
                            }
                            if (i3 >= this.frequency.getValue().intValue()) {
                                break;
                            }
                        } else {
                            continue;
                        }
                    } else {
                        continue;
                    }
                }
            }
            if (this.swap.getValue() == AutoCrystalMode_2.SILENT) {
                FireworksHelper.do456(i2);
            }
            this.stopwatch.reset();
        }
    }

    public boolean is2998(ItemStack itemStack) {
        return itemStack.isOf(Items.NAME_TAG) && itemStack.contains(DataComponentTypes.CUSTOM_NAME);
    }
}
