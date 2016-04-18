package com.beegfs.admon.gui.components.dialogs;

import com.beegfs.admon.gui.common.enums.FilePathsEnum;
import static com.beegfs.admon.gui.common.tools.DefinesTk.DEFAULT_ENCODING_UTF8;
import com.beegfs.admon.gui.common.tools.GuiTk;
import java.awt.Point;
import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.JOptionPane;
import javax.swing.JScrollPane;

public class JDialogEULA extends javax.swing.JDialog
{
   private static final long serialVersionUID = 1L;

   static final Logger logger = Logger.getLogger(JDialogEULA.class.getCanonicalName());

   private boolean retVal;

   /**
    * Creates new form JDialogEULA
    * @param parent The parent frame
    * @param modal True if the dialog should be modal
    */
   public JDialogEULA(java.awt.Frame parent, boolean modal)
   {
      super(parent, modal);
      initComponents();
      this.setIconImage(GuiTk.getFrameIcon().getImage());
      this.setLocationRelativeTo(null);
      retVal = false;

      BufferedReader in = null;
      try
      {
         in = new BufferedReader(new InputStreamReader(JDialogEULA.class.getResource(
            FilePathsEnum.EULA.getPath()).openStream(), DEFAULT_ENCODING_UTF8));

         String line = in.readLine();
         while (line != null)
         {
            jTextAreaEula.append(line + "\n");
            line = in.readLine();
         }
      }
      catch (FileNotFoundException e)
      {
         jTextAreaEula.setText("BeeGFS EULA not found!");
         logger.log(Level.WARNING, "BeeGFS EULA file not found.", e);
      }
      catch (IOException e)
      {
         jTextAreaEula.setText("BeeGFS EULA not readable!");
         logger.log(Level.WARNING, "BeeGFS EULA file not readable.", e);
      }
      finally
      {
         try
         {
            if (in != null)
            {
               in.close();
            }
         }
         catch (IOException | NullPointerException e)
         {
            logger.log(Level.FINEST, "Could not close BeeGFS EULA file.", e);
         }
      }
      
      jScrollPaneEula.getVerticalScrollBar().setValue(0);
      jScrollPaneEula.getHorizontalScrollBar().setValue(0);
      jScrollPaneEula.setViewportView(jTextAreaEula);
      jScrollPaneEula.getViewport().setViewPosition(new Point(10, 10));
      jTextAreaEula.setCaretPosition(0);
   }

   public JScrollPane getScrollPane()
   {
      return jScrollPaneEula;
   }

   /**
    * This method is called from within the constructor to initialize the form. WARNING: Do NOT
    * modify this code. The content of this method is always regenerated by the Form Editor.
    */
   @SuppressWarnings("unchecked")
   // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
   private void initComponents()
   {

      jPanelDialog = new javax.swing.JPanel();
      jPanelControls = new javax.swing.JPanel();
      jCheckBoxAccept = new javax.swing.JCheckBox();
      filler1 = new javax.swing.Box.Filler(new java.awt.Dimension(30, 25), new java.awt.Dimension(30, 25), new java.awt.Dimension(30, 25));
      jButtonAbort = new javax.swing.JButton();
      jButtonOK = new javax.swing.JButton();
      jScrollPaneEula = new javax.swing.JScrollPane();
      jTextAreaEula = new javax.swing.JTextArea();

      setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);
      setPreferredSize(new java.awt.Dimension(607, 600));

      jPanelDialog.setBorder(javax.swing.BorderFactory.createEmptyBorder(10, 10, 10, 10));
      jPanelDialog.setLayout(new java.awt.BorderLayout());

      jPanelControls.setBorder(javax.swing.BorderFactory.createEmptyBorder(10, 10, 0, 10));
      jPanelControls.setLayout(new java.awt.FlowLayout(java.awt.FlowLayout.CENTER, 10, 0));

      jCheckBoxAccept.setText("I accept the terms of the license agreement");
      jCheckBoxAccept.addActionListener(new java.awt.event.ActionListener()
      {
         public void actionPerformed(java.awt.event.ActionEvent evt)
         {
            jCheckBoxAcceptActionPerformed(evt);
         }
      });
      jPanelControls.add(jCheckBoxAccept);
      jPanelControls.add(filler1);

      jButtonAbort.setText("Abort");
      jButtonAbort.setPreferredSize(new java.awt.Dimension(70, 30));
      jButtonAbort.addActionListener(new java.awt.event.ActionListener()
      {
         public void actionPerformed(java.awt.event.ActionEvent evt)
         {
            jButtonAbortActionPerformed(evt);
         }
      });
      jPanelControls.add(jButtonAbort);

      jButtonOK.setText("OK");
      jButtonOK.setPreferredSize(new java.awt.Dimension(70, 30));
      jButtonOK.addActionListener(new java.awt.event.ActionListener()
      {
         public void actionPerformed(java.awt.event.ActionEvent evt)
         {
            jButtonOKActionPerformed(evt);
         }
      });
      jPanelControls.add(jButtonOK);

      jPanelDialog.add(jPanelControls, java.awt.BorderLayout.SOUTH);

      jScrollPaneEula.setViewportBorder(new javax.swing.border.SoftBevelBorder(javax.swing.border.BevelBorder.RAISED));

      jTextAreaEula.setEditable(false);
      jTextAreaEula.setColumns(20);
      jTextAreaEula.setRows(5);
      jScrollPaneEula.setViewportView(jTextAreaEula);

      jPanelDialog.add(jScrollPaneEula, java.awt.BorderLayout.CENTER);

      getContentPane().add(jPanelDialog, java.awt.BorderLayout.CENTER);

      pack();
   }// </editor-fold>//GEN-END:initComponents

    private void jCheckBoxAcceptActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jCheckBoxAcceptActionPerformed

}//GEN-LAST:event_jCheckBoxAcceptActionPerformed

    private void jButtonAbortActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jButtonAbortActionPerformed
       retVal = false;
       dispose();
    }//GEN-LAST:event_jButtonAbortActionPerformed

    private void jButtonOKActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jButtonOKActionPerformed
       if (jCheckBoxAccept.isSelected())
       {
          retVal = true;
          this.dispose();
       }
       else
       {
          JOptionPane.showMessageDialog(null, "You must confirm the BeeGFS Eula in order to install the software.", "License agreement not accepted", JOptionPane.ERROR_MESSAGE);
       }
    }//GEN-LAST:event_jButtonOKActionPerformed

   public boolean showDialog()
   {
      pack();
      setVisible(true);
      return retVal;
   }
   // Variables declaration - do not modify//GEN-BEGIN:variables
   private javax.swing.Box.Filler filler1;
   private javax.swing.JButton jButtonAbort;
   private javax.swing.JButton jButtonOK;
   private javax.swing.JCheckBox jCheckBoxAccept;
   private javax.swing.JPanel jPanelControls;
   private javax.swing.JPanel jPanelDialog;
   private javax.swing.JScrollPane jScrollPaneEula;
   private javax.swing.JTextArea jTextAreaEula;
   // End of variables declaration//GEN-END:variables
}
