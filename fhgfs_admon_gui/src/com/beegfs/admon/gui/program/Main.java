package com.beegfs.admon.gui.program;

import com.beegfs.admon.gui.app.config.Config;
import com.beegfs.admon.gui.app.log.InternalLogger;
import com.beegfs.admon.gui.common.Session;
import com.beegfs.admon.gui.common.enums.FilePathsEnum;
import com.beegfs.admon.gui.components.MainWindow;
import com.beegfs.admon.gui.components.dialogs.JDialogNewConfig;
import com.beegfs.admon.gui.components.lf.BeegfsLookAndFeel;
import java.io.File;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.logging.Handler;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.JOptionPane;
import javax.swing.UIManager;
import javax.swing.UnsupportedLookAndFeelException;


public class Main
{
   private static final String INTERNALLOGGER_LOGGER_NAME = "com.beegfs.admon.gui";
   private static InternalLogger logger;

   private static MainWindow mainWindow;
   private static Config config;
   private static Session session;
   static final double JAVA_MIN_VERSION = 1.7;
   static final String BEEGFS_VERSION = "@VERSION@";
   static final String BEEGFS_JAVA2D_OFFSCREEN = "sun.java2d.pmoffscreen";
   
   /**
    * TODO: Maybe required later, when SecurityManager is implemented
    * static final String BEEGFS_JAVA_POSITIV_DNS_CACHE = "networkaddress.cache.ttl";
    * static final String BEEGFS_JAVA_NEGATIV_DNS_CACHE = "networkaddress.cache.negative.ttl";
    */

   /**
    * @param args the command line arguments
    */
   @SuppressWarnings("UseOfSystemOutOrSystemErr")
   public static void main(String args[])
   {
      boolean connectionSettingsAbort = false;

      resetRootLogger();

      String guiHost = "localhost";
      String javaVersionString = System.getProperty("java.version");
      javaVersionString = javaVersionString.substring(0,3);
      double javaVersion = Double.parseDouble(javaVersionString);

      // try to create the BeeGFS directory if not present
      File f = new File(FilePathsEnum.DIRECTORY.getPath());
      if (!f.exists() )
      {
         if(!f.mkdir() )
         {
            System.err.println("Could not create BeeGFS admon GUI directory: " +
               FilePathsEnum.DIRECTORY.getPath());
         }
      }

      logger = InternalLogger.getLogger(INTERNALLOGGER_LOGGER_NAME);
      logger.log(Level.INFO, "Version: " + BEEGFS_VERSION);

      try
      {
         guiHost = InetAddress.getLocalHost().getHostName();
      }
      catch (UnknownHostException ex)
      {
         logger.log(Level.WARNING, ex.getMessage());
      }
      
      logger.log(Level.INFO, "beegfs-admon-GUI on host {0}", guiHost);
      logger.log(Level.INFO, "Used Java version {0}", String.valueOf(javaVersion));

      if (javaVersion < JAVA_MIN_VERSION)
      {
         System.err.println("Java version " + String.valueOf(javaVersion) + " to low. Java version "
                 + JAVA_MIN_VERSION + " is required.");
         logger.log(Level.SEVERE, "Java version {0} to low. Java version {1} is required.",
            new Object[]{String.valueOf(javaVersion), String.valueOf(JAVA_MIN_VERSION)});
         System.exit(1);
      }

      //disable offscreen pixmap support for X11 Forwording
      System.setProperty(BEEGFS_JAVA2D_OFFSCREEN, "false");
      /**
       * TODO: Maybe required later, when SecurityManager is implemented
       * Security.setProperty(BeeGFS_JAVA_POSITIV_DNS_CACHE, "-1");
       * Security.setProperty(BeeGFS_JAVA_NEGATIV_DNS_CACHE, "-1");
       */

      String osName = System.getProperty("os.name");
      boolean isMacOsx = osName.startsWith("Mac");
      logger.log(Level.INFO, "Used operating system: {0} ", osName);

      // first tell SkinLF which theme to use
      try
      {
         if(isMacOsx)
         { // set Mac OSX specific configuraions

            // take the menu bar off the jframe
            System.setProperty("apple.laf.useScreenMenuBar", "true");

            // set the name of the application menu item
            System.setProperty("com.apple.mrj.application.apple.menu.about.name", "BeeGFS admon");

            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName() );
         }
         else
         { // use BeeGFS look and feel for all other OS
            BeegfsLookAndFeel lf = new BeegfsLookAndFeel();
            UIManager.setLookAndFeel(lf);
         }
      }
      catch (ClassNotFoundException | InstantiationException | IllegalAccessException |
         UnsupportedLookAndFeelException ex)
      {
         logger.log(Level.SEVERE, "Error", ex);
      }

      config = new Config();
      File configFile;
      if (args.length > 0)
      {
         // get the config file from command line
         configFile = new File(args[0]);
         if (!configFile.exists())
         {
            // config file provided by user does not exist
            logger.log(Level.SEVERE, "The configuration file you provided does not exist.");
            JOptionPane.showMessageDialog(null, "The configuration file you provided does not "
                    + "exist.", "Configuration not found", JOptionPane.ERROR_MESSAGE);
            return;
         }
      }
      else
      {
         //no command line specified, try using a default config file
         //try beegfs_dir in users home
         configFile = new File(Config.DEFAULT_CONFIG_NAME);
      }

      if (configFile.exists())
      {
         if (!config.readConfigFile(configFile))
         {
            logger.log(Level.SEVERE, "The configuration file contains errors.");
            JOptionPane.showMessageDialog(null, "The configuration file contains errors.",
                    "Error reading configuration", JOptionPane.ERROR_MESSAGE);
            return;
         }
      }
      else
      {
         JDialogNewConfig newConfig = new JDialogNewConfig(null, true);
         newConfig.setVisible(true);
         connectionSettingsAbort = newConfig.getAbortPushed();
      }

      if (!connectionSettingsAbort)
      {
         logger.setLevel(config.getLogLevel());

         //create session
         session = new Session();
         mainWindow = new MainWindow();

         java.awt.EventQueue.invokeLater(new Runnable()
         {
            @Override
            public void run()
            {
               //    create the GUI
               mainWindow.setVisible(true);
               mainWindow.getLoginDialog().setVisible(true);
            }
         });
      }
   }

   public static MainWindow getMainWindow()
   {
      return Main.mainWindow;
   }

   public static Config getConfig()
   {
      return Main.config;
   }

   public static Session getSession()
   {
      return Main.session;
   }

   public static String getVersion()
   {
      return Main.BEEGFS_VERSION;
   }

   public static void setInternalDesktopResolution(int resolutionX, int resolutionY)
   {
      if (Main.mainWindow != null)
      {
         Main.mainWindow.updateInternalDesktopResolution(resolutionX, resolutionY);
      }
   }

   /*
    * Deletes all handlers from the root logger. If not the console logger isn't disabled.
    */
   public static void resetRootLogger()
   {
      Logger rootLogger = Logger.getLogger("");
      Handler[] allRootHandler = rootLogger.getHandlers();

      for(Handler handler : allRootHandler)
      {
         rootLogger.removeHandler(handler);
      }
   }
}
