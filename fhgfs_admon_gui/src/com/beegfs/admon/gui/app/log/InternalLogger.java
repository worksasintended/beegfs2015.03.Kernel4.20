package com.beegfs.admon.gui.app.log;

import static com.beegfs.admon.gui.app.config.Config.CONFIG_PROPERTY_LOG_DEBUG;
import java.io.IOException;
import java.util.logging.Handler;
import java.util.logging.Level;
import java.util.logging.LogManager;
import java.util.logging.Logger;


public class InternalLogger extends Logger
{
   private static Handler logGuiHandler;
   
   public static InternalLogger getLogger(String name)
   {
      LogManager manager = LogManager.getLogManager();
      Object logger = manager.getLogger(name);

      if (logger == null)
      {
         manager.addLogger(new InternalLogger(name));
      }

      logger = manager.getLogger(name);
      return (InternalLogger)logger;
   }

   protected InternalLogger(String name)
   {
      super(name, null);
      init();
   }

   // Ignore e.printStackTrace() warning, because if logger couln't start a stack trace must be
   // printed to the console
   @SuppressWarnings({"CallToThreadDumpStack", "CallToPrintStackTrace", "UseOfSystemOutOrSystemErr"})
   private void init()
   {
      try
      {
         // This block configures the logger with handler and formatter
         boolean logDebug = Boolean.parseBoolean(System.getProperty(CONFIG_PROPERTY_LOG_DEBUG));

         if (logDebug)
         {
            setLevel(Level.ALL);
         }
         else
         {
            setLevel(Level.INFO);
         }

         logGuiHandler = new LogHandlerGui();
         addHandler(logGuiHandler);

         setUseParentHandlers(true);
      }
      catch (IOException e)
      {
         // logging only to system.err possible
         System.err.println(e.getMessage() + "\n");
         e.printStackTrace();
      }
   }
}
