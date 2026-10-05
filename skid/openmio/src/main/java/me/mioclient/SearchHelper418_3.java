package me.mioclient;

import net.minecraft.item.ArmorItem;
import net.minecraft.item.Item;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper418_3.class */
public class SearchHelper418_3 implements SearchHelper4_18 {
    @Override // me.mioclient.SearchHelper4_18
    public void do251(Data_4 data_4) {
        FireworksHelper.do441(data_4.get1776(), data_4.get1777());
    }

    @Override // me.mioclient.SearchHelper4_18
    public void do252(Data_4 data_4) {
        FireworksHelper.do441(data_4.get1777(), data_4.get1776());
    }

    @Override // me.mioclient.SearchHelper4_18
    public Data_4 getData_4253(Item item) {
        if (!Feature_9.is1295(item)) {
            return Data_4.data_4;
        }
        int i = 6;
        if (item instanceof ArmorItem) {
            i = 8 - ((ArmorItem) item).getSlotType().getEntitySlotId();
        }
        return new Data_4(FireworksHelper.get443(item), i);
    }
}
