package me.mioclient;

import net.minecraft.item.Item;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper418_2.class */
public class SearchHelper418_2 implements SearchHelper4_18 {
    @Override // me.mioclient.SearchHelper4_18
    public void do251(Data_4 data_4) {
        FireworksHelper.do438(data_4.get1776());
    }

    @Override // me.mioclient.SearchHelper4_18
    public void do252(Data_4 data_4) {
        FireworksHelper.do438(data_4.get1777());
    }

    @Override // me.mioclient.SearchHelper4_18
    public Data_4 getData_4253(Item item) {
        return new Data_4(FireworksHelper.get447(item), minecraftClient.player.getInventory().selectedSlot);
    }
}
