package me.mioclient;

import net.minecraft.item.Item;
import net.minecraft.screen.slot.SlotActionType;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper418_4.class */
public class SearchHelper418_4 implements SearchHelper4_18 {
    @Override // me.mioclient.SearchHelper4_18
    public void do251(Data_4 data_4) {
        minecraftClient.interactionManager.clickSlot(minecraftClient.player.currentScreenHandler.syncId, data_4.get1777(), 40, SlotActionType.SWAP, minecraftClient.player);
    }

    @Override // me.mioclient.SearchHelper4_18
    public void do252(Data_4 data_4) {
        minecraftClient.interactionManager.clickSlot(minecraftClient.player.currentScreenHandler.syncId, data_4.get1777(), 40, SlotActionType.SWAP, minecraftClient.player);
    }

    @Override // me.mioclient.SearchHelper4_18
    public Data_4 getData_4253(Item item) {
        return new Data_4(FireworksHelper.get443(item), 40);
    }
}
