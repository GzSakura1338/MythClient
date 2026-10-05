package me.mioclient;

import me.mioclient.module.combat.AutoMine;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.BlockPos;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/AutoMineSearchHelper42_2.class */
public final class AutoMineSearchHelper42_2 extends AutoMineSearchHelper4_2 {
    public AutoMineSearchHelper42_2(AutoMine autoMine) {
        super(autoMine);
    }

    @Override // me.mioclient.AutoMineHelper_2
    public void do722(AutoMineHelper autoMineHelper) {
        LivingEntity playerEntity886 = this.autoMine.autoMineSearchHelper4.getPlayerEntity886();
        if (playerEntity886 != null && BlockPos.stream(this.autoMine.autoMineSearchHelper4.getBox2244((PlayerEntity) playerEntity886)).count() == 1) {
            BlockPos blockPos2008 = HoleSnapSearchHelper4.getBlockPos2008(playerEntity886);
            if (SearchHelper4_7.is2446(blockPos2008) || SearchHelper4_7.is2435(blockPos2008) || !SearchHelper4_7.is2435(blockPos2008.down())) {
                return;
            }
            autoMineHelper.do2901(400, autoMineHelper2 -> {
                if (this.autoMine.is2754()) {
                    autoMineHelper2.do2899(blockPos2008.down());
                } else {
                    autoMineHelper2.do667(blockPos2008.down());
                }
            });
        }
    }

    @Override // me.mioclient.AutoMineSearchHelper4_2, me.mioclient.AutoMineHelper_2
    public boolean is465() {
        return this.autoMine.fish.getValue().booleanValue();
    }
}
