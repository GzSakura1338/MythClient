package me.mioclient;

import java.util.Iterator;
import java.util.List;
import me.mioclient.api.Keybind;

public class KeybindFeature_2 extends KeybindFeature {
   public KeybindFeature_2(String var1, Keybind var2) {
      super(var1, Mode_4.SIMPLE, var2);
   }

   @Override
   public void run() {
      List var3 = this.list;
      Iterator var1 = var3.iterator();

      while (true) {
         Iterator var5 = var1;
         if (!var5.hasNext()) {
            return;
         }

         Iterator var6 = var1;
         String var2 = (String)var6.next();
         String var4 = var2;
         KeybindFeature_2 var7 = this;
         var7.do2060(var4);
      }
   }
}
