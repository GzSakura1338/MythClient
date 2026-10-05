package me.mioclient.loader;

import java.io.PrintWriter;
import java.io.StringWriter;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

public final class Globals {
   private static final Logger logger = LogManager.getLogger("turboloader");

   public static void printMessage(String string) {
      logger.info("[turboloader] {}", string);
   }

   public static void warn(String string) {
      logger.warn("[turboloader] {}", string);
   }

   public static void printException(Exception exception) {
      PrintWriter writer = new PrintWriter(new StringWriter());
      exception.printStackTrace(writer);
      logger.error(writer.toString());
   }
}
