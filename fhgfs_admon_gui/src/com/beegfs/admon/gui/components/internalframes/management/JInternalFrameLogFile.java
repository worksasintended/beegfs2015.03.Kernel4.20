package com.beegfs.admon.gui.components.internalframes.management;

import com.beegfs.admon.gui.common.XMLParser;
import com.beegfs.admon.gui.common.exceptions.CommunicationException;
import static com.beegfs.admon.gui.common.tools.DefinesTk.DEFAULT_ENCODING_UTF8;
import com.beegfs.admon.gui.common.tools.GuiTk;
import com.beegfs.admon.gui.common.tools.HttpTk;
import com.beegfs.admon.gui.components.internalframes.JInternalFrameInterface;
import com.beegfs.admon.gui.components.managers.FrameManager;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;


public class JInternalFrameLogFile extends javax.swing.JInternalFrame implements
   JInternalFrameInterface
{
   static final Logger LOGGER = Logger.getLogger(JInternalFrameLogFile.class.getCanonicalName());
   private static final long serialVersionUID = 1L;
   private static final String THREAD_NAME = "LoadLogFile";

   /** Creates new form JInternalFrameMetaNode */
   public JInternalFrameLogFile()
   {
      initComponents();
      setTitle(getFrameTitle());
      setFrameIcon(GuiTk.getFrameIcon());
        
      loadLogFile();
   }

   @Override
   public boolean isEqual(JInternalFrameInterface obj)
   {
      return obj instanceof JInternalFrameLogFile;
   }

   private void loadLogFile()
   {
      jTextPaneLogfile.setText("Waiting for data...");
      final XMLParser parser = new XMLParser(HttpTk.generateAdmonUrl("/XML_LogFile"), true,
         THREAD_NAME);
      parser.start();
      ActionListener taskPerformer = new ActionListener()
      {
         @Override
         public void actionPerformed(ActionEvent evt)
         {
            try
            {
               if (!parser.getValue("success").isEmpty())
               {
                  String log = parser.getValue("log");
                  jTextPaneLogfile.setText(log);
                  javax.swing.Timer t = (javax.swing.Timer)evt.getSource();
                  t.stop();
               }
            } 
            catch (CommunicationException e)
            {
               LOGGER.log(Level.SEVERE, "Communication Error occured", new Object[]
               {
                  e,
                  true
               });
            }
         }
      };
      new javax.swing.Timer(1000, taskPerformer).start();
   }

   /** This method is called from within the constructor to
    * initialize the form.
    * WARNING: Do NOT modify this code. The content of this method is
    * always regenerated by the Form Editor.
    */
   @SuppressWarnings("unchecked")
   // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
   private void initComponents()
   {

      jScrollPaneFrame = new javax.swing.JScrollPane();
      jPanelFrame = new javax.swing.JPanel();
      jPanelButtons = new javax.swing.JPanel();
      jButtonReload = new javax.swing.JButton();
      filler1 = new javax.swing.Box.Filler(new java.awt.Dimension(400, 30), new java.awt.Dimension(400, 30), new java.awt.Dimension(400, 30));
      jButtonSave = new javax.swing.JButton();
      jScrollPaneLogfile = new javax.swing.JScrollPane();
      jTextPaneLogfile = new javax.swing.JTextPane();

      setClosable(true);
      setIconifiable(true);
      setMaximizable(true);
      setResizable(true);
      addInternalFrameListener(new javax.swing.event.InternalFrameListener()
      {
         public void internalFrameOpened(javax.swing.event.InternalFrameEvent evt)
         {
         }
         public void internalFrameClosing(javax.swing.event.InternalFrameEvent evt)
         {
         }
         public void internalFrameClosed(javax.swing.event.InternalFrameEvent evt)
         {
            formInternalFrameClosed(evt);
         }
         public void internalFrameIconified(javax.swing.event.InternalFrameEvent evt)
         {
         }
         public void internalFrameDeiconified(javax.swing.event.InternalFrameEvent evt)
         {
         }
         public void internalFrameActivated(javax.swing.event.InternalFrameEvent evt)
         {
         }
         public void internalFrameDeactivated(javax.swing.event.InternalFrameEvent evt)
         {
         }
      });

      jPanelFrame.setBorder(javax.swing.BorderFactory.createEmptyBorder(10, 10, 10, 10));
      jPanelFrame.setPreferredSize(new java.awt.Dimension(792, 409));
      jPanelFrame.setLayout(new java.awt.BorderLayout(5, 5));

      jButtonReload.setText("Reload");
      jButtonReload.setMaximumSize(new java.awt.Dimension(100, 30));
      jButtonReload.setMinimumSize(new java.awt.Dimension(100, 30));
      jButtonReload.setPreferredSize(new java.awt.Dimension(100, 30));
      jButtonReload.addActionListener(new java.awt.event.ActionListener()
      {
         public void actionPerformed(java.awt.event.ActionEvent evt)
         {
            jButtonReloadActionPerformed(evt);
         }
      });
      jPanelButtons.add(jButtonReload);
      jPanelButtons.add(filler1);

      jButtonSave.setText("Save to file");
      jButtonSave.setMaximumSize(new java.awt.Dimension(100, 30));
      jButtonSave.setMinimumSize(new java.awt.Dimension(100, 30));
      jButtonSave.setPreferredSize(new java.awt.Dimension(100, 30));
      jButtonSave.addActionListener(new java.awt.event.ActionListener()
      {
         public void actionPerformed(java.awt.event.ActionEvent evt)
         {
            jButtonSaveActionPerformed(evt);
         }
      });
      jPanelButtons.add(jButtonSave);

      jPanelFrame.add(jPanelButtons, java.awt.BorderLayout.SOUTH);

      jTextPaneLogfile.setEditable(false);
      jScrollPaneLogfile.setViewportView(jTextPaneLogfile);

      jPanelFrame.add(jScrollPaneLogfile, java.awt.BorderLayout.CENTER);

      jScrollPaneFrame.setViewportView(jPanelFrame);

      javax.swing.GroupLayout layout = new javax.swing.GroupLayout(getContentPane());
      getContentPane().setLayout(layout);
      layout.setHorizontalGroup(
         layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
         .addComponent(jScrollPaneFrame, javax.swing.GroupLayout.DEFAULT_SIZE, 798, Short.MAX_VALUE)
      );
      layout.setVerticalGroup(
         layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
         .addComponent(jScrollPaneFrame, javax.swing.GroupLayout.DEFAULT_SIZE, 450, Short.MAX_VALUE)
      );

      pack();
   }// </editor-fold>//GEN-END:initComponents

    private void formInternalFrameClosed(javax.swing.event.InternalFrameEvent evt) {//GEN-FIRST:event_formInternalFrameClosed
      //  parser.shouldStop();
      FrameManager.delFrame(this);
    }//GEN-LAST:event_formInternalFrameClosed

    private void jButtonReloadActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jButtonReloadActionPerformed
      loadLogFile();
    }//GEN-LAST:event_jButtonReloadActionPerformed

    private void jButtonSaveActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jButtonSaveActionPerformed
      JFileChooser chooser = new JFileChooser();
      chooser.setMultiSelectionEnabled(false);
      chooser.setDialogTitle("Save log file");
      chooser.setDialogType(JFileChooser.SAVE_DIALOG);
      boolean showDialog = true;
      boolean writeFile = false;
      while (showDialog)
      {
         int retVal = chooser.showSaveDialog(this);
         if (retVal == JFileChooser.APPROVE_OPTION)
         {
            File f = chooser.getSelectedFile();
            if (f.exists())
            {
               if (JOptionPane.showConfirmDialog(null,"The file " + f.getName() +
                  " exists. Do you really want to overwrite it?", "File exists",
                  JOptionPane.YES_NO_OPTION) == JOptionPane.OK_OPTION)
               {
                  if ( f.canWrite() )
                  {
                     writeFile = true;
                     showDialog = false;
                  }
                  else
                  {
                     JOptionPane.showMessageDialog(this, "The file " + f.getName() +
                        " can not be written! Do you have permissions to write?",
                        "File cannot be written", JOptionPane.ERROR_MESSAGE);
                     showDialog = true;
                  }
               }
            }
            else
            {
               if (f.getParentFile().canWrite())
               {
                  writeFile = true;
                  showDialog = false;
               }
               else
               {
                  JOptionPane.showMessageDialog(this, "The file " + f.getName() +
                     " can not be written! Do you have permissions to write?",
                     "File cannot be written", JOptionPane.ERROR_MESSAGE);
                  showDialog = true;
               }
            }

            if (writeFile)
            {
               BufferedWriter bw = null;
               try
               {
                  bw = new BufferedWriter(new OutputStreamWriter(
                     new FileOutputStream(chooser.getSelectedFile()), DEFAULT_ENCODING_UTF8));
                  bw.write(jTextPaneLogfile.getText());
                  bw.close();
               } catch (IOException e) {
                  LOGGER.log(Level.SEVERE, "IO Exception occured while saving log file",
                     new Object[]{e, true});
               }
               finally
               {
                  if (bw != null)
                  {
                     try
                     {
                        bw.close();
                     }
                     catch (IOException ex)
                     {
                        LOGGER.log(Level.SEVERE, "IO Exception occured while saving log file",
                           new Object[]{ex, true});
                     }
                  }
               }
            }
         }
         else
         {
            showDialog = false;
         }
      }
    }//GEN-LAST:event_jButtonSaveActionPerformed


   // Variables declaration - do not modify//GEN-BEGIN:variables
   private javax.swing.Box.Filler filler1;
   private javax.swing.JButton jButtonReload;
   private javax.swing.JButton jButtonSave;
   private javax.swing.JPanel jPanelButtons;
   private javax.swing.JPanel jPanelFrame;
   private javax.swing.JScrollPane jScrollPaneFrame;
   private javax.swing.JScrollPane jScrollPaneLogfile;
   private javax.swing.JTextPane jTextPaneLogfile;
   // End of variables declaration//GEN-END:variables

   @Override
   public final String getFrameTitle()
   {
      return "BeeGFS Installation Log File";
   }

}
