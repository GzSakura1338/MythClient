package me.mioclient.module.combat;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.EnumSettingHelper;
import me.mioclient.FireworksHelper;
import me.mioclient.HoleFillHelper;
import me.mioclient.HoleFillHelper2;
import me.mioclient.HoleFillHelper_2;
import me.mioclient.HoleSnapData;
import me.mioclient.HoleSnapSearchHelper4;
import me.mioclient.PhaseESPHelper;
import me.mioclient.api.Category;
import me.mioclient.api.Setting;
import me.mioclient.module.Delay;
import net.minecraft.block.Block;
import net.minecraft.block.Blocks;
import net.minecraft.item.BlockItem;
import net.minecraft.item.ItemStack;
import net.minecraft.util.math.BlockPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/module/combat/HoleFill.class */
public class HoleFill extends Delay {
    public Setting<Set<Block>> whitelist;
    public Setting<Boolean> doubles;
    public Setting<HoleFillPredicateMode> mode;
    public Setting<Float> smartRange;
    public Setting<Float> range;
    public Setting<Float> verticalRange;
    public Setting<Boolean> onlySafe;
    public Setting<Boolean> selfSafety;
    public Setting<Boolean> ignoreNakeds;
    public Setting<Boolean> multiTask;
    public Setting<Boolean> self;
    public Setting<Boolean> extrapolation;
    public Setting<Integer> ticks;
    public final HoleFillHelper holeFillHelper;
    public final HoleFillHelper holeFillHelper2;

    /* JADX INFO: Access modifiers changed from: private */
    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/module/combat/HoleFill$HoleFillPredicateMode.class */
    public enum HoleFillPredicateMode implements EnumSettingHelper {
        PLAIN("Plain"),
        SMART("Smart");

        public final String name;

        HoleFillPredicateMode(String str) {
            this.name = str;
        }

        @Override // me.mioclient.EnumSettingHelper
        public String getName() {
            return this.name;
        }

        public HoleFillHelper getHoleFillHelper1726(HoleFill holeFill) {
            return this == PLAIN ? holeFill.holeFillHelper : holeFill.holeFillHelper2;
        }
    }

    public HoleFill() {
        super("HoleFill", "Blocks safe holes nearby with obsidian.", Category.COMBAT, new String[0]);
        PhaseESPHelper.do1351(this);
        this.holeFillHelper = new HoleFillHelper_2(this);
        this.holeFillHelper2 = new HoleFillHelper2(this);
        unregister((Setting<?>) this.setting3);
        unregister((Setting<?>) this.setting10);
        this.setting9.do2334(false);
        this.self.do2343(bool -> {
            Iterator<Block> it = this.whitelist.getValue().iterator();
            while (it.hasNext()) {
                if (!((me.mioclient.mixin.ducks.DuckAbstractBlock) (Object) it.next()).isCollidable()) {
                    return true;
                }
            }
            return false;
        });
    }

    @Override // me.mioclient.module.Module
    public String getInfo() {
        return String.valueOf(getList876().size());
    }

    @Override // me.mioclient.module.Delay
    public void do1323() {
        if (!minecraftClient.player.isUsingItem() || this.multiTask.getValue().booleanValue()) {
            if (BaritoneHelper_3.holeSnapSearchHelper4_5.is2728() || !this.onlySafe.getValue().booleanValue()) {
                super.do1323();
            }
        }
    }

    @Override // me.mioclient.module.Delay
    public List<BlockPos> getList876() {
        ArrayList arrayList = new ArrayList();
        for (HoleSnapData holeSnapData : BaritoneHelper_3.holeSnapSearchHelper4_5.getList2726()) {
            if (this.mode.getValue().getHoleFillHelper1726(this).is464(holeSnapData) && ((this.self.getValue().booleanValue() && this.self.is2349()) || !holeSnapData.getBlockPos12().equals(HoleSnapSearchHelper4.getBlockPos1333()))) {
                arrayList.add(holeSnapData.getBlockPos12());
            }
        }
        return arrayList;
    }

    @Override // me.mioclient.module.Delay
    public int get499() {
        return this.whitelist.getValue().isEmpty() ? super.get499() : FireworksHelper.get448(itemStack -> {
            return this.whitelist.getValue().contains(getBlock2756(itemStack));
        });
    }

    public Block getBlock2756(ItemStack itemStack) {
        BlockItem item = (itemStack.getItem()) instanceof BlockItem ? (BlockItem) (itemStack.getItem()) : null;
        return item instanceof BlockItem ? item.getBlock() : Blocks.VOID_AIR;
    }
}
