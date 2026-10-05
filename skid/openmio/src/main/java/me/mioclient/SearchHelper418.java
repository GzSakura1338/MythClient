package me.mioclient;

import net.minecraft.item.Item;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper418.class */
public class SearchHelper418 implements SearchHelper4_18 {
    @Override // me.mioclient.SearchHelper4_18
    public void do251(Data_4 data_4) {
        FireworksHelper.do439(data_4.get1776());
    }

    @Override // me.mioclient.SearchHelper4_18
    public void do252(Data_4 data_4) {
        FireworksHelper.do439(data_4.get1776());
    }

    @Override // me.mioclient.SearchHelper4_18
    public Data_4 getData_4253(Item item) {
        int i = FireworksHelper.get453(FireworksHelper.get447(item));
        if (i == -1) {
            i = FireworksHelper.get443(item);
        }
        return new Data_4(i, minecraftClient.player.getInventory().selectedSlot);
    }
}
