package me.mioclient;

import java.util.Iterator;
import java.util.List;
import me.mioclient.api.Keybind;
import me.mioclient.feature.Stopwatch;

public class StopwatchKeybindFeature extends KeybindFeature {
   public final Stopwatch stopwatch = new Stopwatch();

   public StopwatchKeybindFeature(String var1, Keybind var2) {
      super(var1, Mode_4.DOUBLE_TAP, var2);
   }

   @Override
   public void run() {
      long var4 = 300L;
      Stopwatch var3 = this.stopwatch;
      if (var3.is419(var4)) {
         var3 = this.stopwatch;
         var3.reset();
      } else {
         List var6 = this.list;
         Iterator var1 = var6.iterator();

         while (true) {
            Iterator var7 = var1;
            if (!var7.hasNext()) {
               var4 = -1L;
               var3 = this.stopwatch;
               var3.setTime(var4);
               return;
            }

            Iterator var8 = var1;
            String var2 = (String)var8.next();
            String var12 = var2;
            StopwatchKeybindFeature var9 = this;
            var9.do2060(var12);
         }
      }
   }
}
