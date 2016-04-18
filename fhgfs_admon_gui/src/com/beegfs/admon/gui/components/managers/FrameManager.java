package com.beegfs.admon.gui.components.managers;

import com.beegfs.admon.gui.components.internalframes.JInternalFrameInterface;
import java.beans.PropertyVetoException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.JToggleButton;

public class FrameManager
{
   static final Logger LOGGER = Logger.getLogger(FrameManager.class.getCanonicalName());

   private final static int INITIAL_CAPACITY = 25;
   private final static HashMap<JInternalFrameInterface, JToggleButton> OPEN_FRAMES =
      new HashMap<>(INITIAL_CAPACITY);

   public static void addFrame(JInternalFrameInterface frame)
   {
      JToggleButton button = new JToggleButton();
      OPEN_FRAMES.put(frame, button);
   }

   public static boolean delFrame(JInternalFrameInterface removeFrame)
   {
      try
      {
         HashSet<JInternalFrameInterface> keySet = new HashSet<>(OPEN_FRAMES.keySet());
         for (JInternalFrameInterface frame : keySet)
         {
            if (frame.isEqual(removeFrame))
            {
               FrameManager.getOpenFrame(frame);
               OPEN_FRAMES.remove(frame);
               return true;
            }
         }
         return false;
      }
      catch (java.lang.NullPointerException e)
      {
         LOGGER.log(Level.FINEST, "Internal error", e);
         return false;
      }
   }

   public static boolean isFrameOpen(JInternalFrameInterface newFrame)
   {
      HashSet<JInternalFrameInterface> keySet = new HashSet<>(OPEN_FRAMES.keySet());
      try
      {
         for (JInternalFrameInterface frame : keySet)
         {
            if (frame.isEqual(newFrame))
            {
               FrameManager.getOpenFrame(frame);
               return true;
            }
         }
      }
      catch (java.lang.NullPointerException e)
      {
         LOGGER.log(Level.FINEST, "Internal error.", e);
      }
      return false;
   }

   public static JInternalFrameInterface getOpenFrame(JInternalFrameInterface newFrame)
   {
      HashSet<JInternalFrameInterface> keySet = new HashSet<>(OPEN_FRAMES.keySet());
      try
      {
         for (JInternalFrameInterface frame : keySet)
         {
            if (frame.isEqual(newFrame))
            {
               JInternalFrameInterface f = frame;
               try
               {
                  f.setIcon(false);
                  f.toFront();
               }
               catch (PropertyVetoException ex)
               {
                  LOGGER.log(Level.FINEST, "Internal error.", ex);
               }
               return frame;
            }
         }
      }
      catch (java.lang.NullPointerException e)
      {
         LOGGER.log(Level.FINEST, "Internal error.", e);
      }
      return null;
   }

   private FrameManager()
   {
   }
}
