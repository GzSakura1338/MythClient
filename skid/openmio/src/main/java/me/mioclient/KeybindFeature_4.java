package me.mioclient;

import java.util.List;
import me.mioclient.api.Keybind;

public class KeybindFeature_4 extends KeybindFeature {
   public int num = 0;

   public KeybindFeature_4(String var1, Keybind var2) {
      super(var1, Mode_4.QUEUE, var2);
   }

   @Override
   public void run() {
      List var7 = this.list;
      if (!var7.isEmpty()) {
         var7 = this.list;
         this.num = this.num % var7.size();
         int var8 = this.num;
         var7 = this.list;
         String var1 = (String)var7.get(var8);
         String var13 = ";";
         String var11 = var1;
         String[] var2 = var11.split(var13);

         for (String var6 : var2) {
            String var14 = var6;
            KeybindFeature_4 var12 = this;
            var12.do2060(var14);
         }

         this.num++;
      }
   }
}
