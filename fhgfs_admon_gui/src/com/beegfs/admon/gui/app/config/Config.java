package com.beegfs.admon.gui.app.config;

import com.beegfs.admon.gui.common.enums.FilePathsEnum;
import static com.beegfs.admon.gui.common.tools.DefinesTk.DEFAULT_ENCODING_UTF8;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.Logger;


public class Config
{
   static final Logger LOGGER = Logger.getLogger(Config.class.getCanonicalName());

   public static final String DEFAULT_CONFIG_NAME = FilePathsEnum.CONFIG_FILE.getPath();

   public static final String CONFIG_PROPERTY_LOG_DEBUG = "logDebug";

   private final String CONFIG_PROPERTY_ADMON_HOST = "admonHost";
   private final String CONFIG_PROPERTY_ADMON_HTTP_PORT = "admonHttpPort";
   private final String CONFIG_PROPERTY_LOG_LEVEL = "logLevel";
   private final String CONFIG_PROPERTY_RESOLUTION = "resolution";

   private final String DEFAULT_ADMON_HOST = "localhost";
   private final String DEFAULT_HTTP_PORT = "8000";
   private final int DEFAULT_RESOLUTION_X = 2048;
   private final int DEFAULT_RESOLUTION_Y = 2048;
   private final Level DEFAULT_LOG_LEVEL = Level.INFO;

   private String admonHost = DEFAULT_ADMON_HOST;
   private String admonHttpPort = DEFAULT_HTTP_PORT;
   private int resolutionX = DEFAULT_RESOLUTION_X;
   private int resolutionY = DEFAULT_RESOLUTION_Y;
   private Level logLevel = DEFAULT_LOG_LEVEL;

   public Config()
   {
      boolean logDebug = Boolean.parseBoolean(System.getProperty(CONFIG_PROPERTY_LOG_DEBUG));

      if (logDebug)
      {
         logLevel = Level.ALL;
      }
      else
      {
         this.logLevel = DEFAULT_LOG_LEVEL;
      }
   }

   public String getAdmonHost()
   {
      return this.admonHost;
   }

   public String getAdmonHttpPort()
   {
      return this.admonHttpPort;
   }

   public String getResolution()
   {
      return this.resolutionX + "x" + this.resolutionY;
   }

   public int getResolutionX()
   {
      return this.resolutionX;
   }

   public int getResolutionY()
   {
      return this.resolutionY;
   }

   public Level getLogLevel()
   {
      return this.logLevel;
   }

   public int getLogLevelNumeric()
   {
      if (this.logLevel == Level.SEVERE)
      {
         return 0;
      }
      else if (this.logLevel == Level.SEVERE)
      {
         return 1;
      }
      else if (this.logLevel == Level.WARNING)
      {
         return 2;
      }
      else if (this.logLevel == Level.INFO)
      {
         return 3;
      }
      else if (this.logLevel == Level.CONFIG)
      {
         return 3;
      }
      else if (this.logLevel == Level.FINE)
      {
         return 4;
      }
      else if (this.logLevel == Level.FINER)
      {
         return 5;
      }
      else if (this.logLevel == Level.FINEST)
      {
         return 5;
      }
      return 3;
   }

   public void setAdmonHost(String host)
   {
      this.admonHost = host;
   }

   public void setAdmonHttpPort(String port)
   {
      admonHttpPort = port;
   }

   public boolean setResolution(String resolution)
   {
      boolean retVal = false;

      String[] splittedResolution = resolution.split("x");
      if (splittedResolution.length == 2)
      {
         try
         {
            this.resolutionX = Integer.parseInt(splittedResolution[0]);
            this.resolutionY = Integer.parseInt(splittedResolution[1]);

            retVal = true;
         }
         catch (NumberFormatException ex)
         {
            LOGGER.log(Level.INFO, "Resolution value isn't valid.", ex);
            retVal = false;
         }
      }

      return retVal;
   }

   public void setResolutionX(int resolutionX)
   {
      this.resolutionX = resolutionX;
   }

   public void setResolutionY(int resolutionY)
   {
      this.resolutionY = resolutionY;
   }

   public void setLogLevel(int logLevel)
   {
      switch (logLevel)
      {
         case 0:
         case 1:
            this.logLevel = Level.SEVERE;
            break;
         case 2:
            this.logLevel = Level.WARNING;
            break;
         case 3:
            this.logLevel = Level.CONFIG;
            break;
         case 4:
            this.logLevel = Level.FINE;
            break;
         case 5:
            this.logLevel = Level.FINEST;
            break;
         default:
      }
   }

   public void setLogLevel(Level logLevel)
   {
      this.logLevel = logLevel;
   }

   public boolean readConfigFile(File f)
   {
      boolean retVal = false;
      FileInputStream fr = null;
      InputStreamReader streamReader = null;

      try
      {
         fr = new FileInputStream(f);
         Properties props = new Properties();
         streamReader = new InputStreamReader(fr, DEFAULT_ENCODING_UTF8);
         props.load(streamReader);

         String newHost = props.getProperty(CONFIG_PROPERTY_ADMON_HOST);
         String newPort = props.getProperty(CONFIG_PROPERTY_ADMON_HTTP_PORT);
         String newResolution = props.getProperty(CONFIG_PROPERTY_RESOLUTION);
         String newLogLevel = props.getProperty(CONFIG_PROPERTY_LOG_LEVEL);
         if ((newHost != null) || (newPort != null))
         {
            this.admonHost = newHost;
            this.admonHttpPort = newPort;
            retVal = true;
         }
         else
         {
            this.admonHost = DEFAULT_ADMON_HOST;
            this.admonHttpPort = DEFAULT_HTTP_PORT;
            retVal = true;
         }

         if (newResolution != null)
         {
            if (!setResolution(newResolution))
            {
               this.resolutionX = DEFAULT_RESOLUTION_X;
               this.resolutionY = DEFAULT_RESOLUTION_Y;
            }
         }
         else
         {
            this.resolutionX = DEFAULT_RESOLUTION_X;
            this.resolutionY = DEFAULT_RESOLUTION_Y;
         }

         if (newLogLevel != null)
         {
            setLogLevel(Integer.parseInt(newLogLevel));
         }
         else
         {  
            this.logLevel = DEFAULT_LOG_LEVEL;
         }
      }
      catch (IOException ex)
      {
         LOGGER.log(Level.WARNING, "IO error. Could not read configuration file, using defaults.",
            ex);

         this.admonHost = DEFAULT_ADMON_HOST;
         this.admonHttpPort = DEFAULT_HTTP_PORT;
         this.resolutionX = DEFAULT_RESOLUTION_X;
         this.resolutionY = DEFAULT_RESOLUTION_Y;
         this.logLevel = DEFAULT_LOG_LEVEL;
      }
      finally
      {
         if (fr != null)
         {
            try
            {             
               fr.close();
            }
            catch (IOException e)
            {
               LOGGER.log(Level.FINEST, "Could not close configuration file.", e);
            }
         }

         if (streamReader != null)
         {
            try
            {
               streamReader.close();
            }
            catch (IOException ex)
            {
               LOGGER.log(Level.FINEST, "Could not close configuration file.", ex);
            }
         }
      }
      return retVal;
   }

   public boolean writeConfigFile(File f, Properties p)
   {
      boolean retVal = false;
      FileOutputStream fw = null;
      OutputStreamWriter streamWriter = null;

      try
      {
         fw = new FileOutputStream(f);
         streamWriter = new OutputStreamWriter(fw, DEFAULT_ENCODING_UTF8);
         p.store(streamWriter, "");
         retVal = true;
      }
      catch (IOException ex)
      {
         LOGGER.log(Level.SEVERE, "Could not write configuration file.", ex);
      }
      finally
      {
         if (fw != null)
         {
            try
            {
               fw.close();
            }
            catch (IOException e)
            {
               LOGGER.log(Level.FINEST, "Could not close configuration file.", e);
            }
         }

         if (streamWriter != null)
         {
            try
            {
               streamWriter.close();
            }
               catch (IOException ex)
               {
                  LOGGER.log(Level.FINEST, "Could not close configuration file.", ex);
               }
            }
      }
      return retVal;
   }

   public boolean writeConfigFile(File f)
   {
      Properties p = new Properties();
      p.put(CONFIG_PROPERTY_ADMON_HOST, this.admonHost);
      p.put(CONFIG_PROPERTY_ADMON_HTTP_PORT, this.admonHttpPort);
      p.put(CONFIG_PROPERTY_LOG_LEVEL, String.valueOf(getLogLevelNumeric()));
      p.put(CONFIG_PROPERTY_RESOLUTION, getResolution());
      return writeConfigFile(f,p);
   }
}
