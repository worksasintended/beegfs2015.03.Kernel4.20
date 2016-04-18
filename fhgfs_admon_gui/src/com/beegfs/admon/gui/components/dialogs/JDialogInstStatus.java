package com.beegfs.admon.gui.components.dialogs;

import com.beegfs.admon.gui.common.enums.PropertyEnum;
import com.beegfs.admon.gui.common.threading.GuiThread;
import com.beegfs.admon.gui.common.tools.GuiTk;
import com.beegfs.admon.gui.program.Main;
import java.awt.Color;
import java.awt.Dimension;
import javax.swing.JButton;
import javax.swing.JProgressBar;

public class JDialogInstStatus extends javax.swing.JDialog
{
   private static final long serialVersionUID = 1L;

   private transient GuiThread managementThread;
   private boolean readyToClose;

   /**
    * Creates new form JDialogInstStatus
    */
   public JDialogInstStatus()
   {
      super(Main.getMainWindow(), false);
      this.setIconImage(GuiTk.getFrameIcon().getImage());
      this.setLocationRelativeTo(null);
      managementThread = null;
      readyToClose = false;
      initComponents();
      this.jProgressBar.setVisible(false);
      this.setTitle("Install status");
   }

   public JProgressBar getProgressBar()
   {
      return this.jProgressBar;
   }

   public void addLine(String text, boolean error)
   {
      if (error)
      {
         jTextArea1.setBackground(Color.red);
      }

      jTextArea1.append(text + System.getProperty(PropertyEnum.PROPERTY_LINE_SEPARATOR.getKey()));
   }

   public void addLine(String text)
   {
      addLine(text, false);
   }

   public void setFinished()
   {
      Dimension size = jButtonAbort.getSize();
      jButtonAbort.setText("Close");
      jButtonAbort.setHorizontalAlignment(JButton.CENTER);
      jButtonAbort.setSize(size);
      readyToClose = true;

      addLine("  ");
      addLine("If you need any support please attach the BeeGFS installation");
      addLine("log file, which can be accessed from the main menu");
      addLine("(Installation -> Installation Log File) to your request");
   }

   public void setManagementThread(GuiThread thread)
   {
      this.managementThread = thread;
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
      jScrollPaneStatus = new javax.swing.JScrollPane();
      jTextArea1 = new javax.swing.JTextArea();
      jProgressBar = new javax.swing.JProgressBar();
      jButtonAbort = new javax.swing.JButton();

      setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);
      setMinimumSize(new java.awt.Dimension(500, 400));
      setPreferredSize(new java.awt.Dimension(537, 465));

      jPanelDialog.setBorder(javax.swing.BorderFactory.createEmptyBorder(10, 10, 10, 10));
      jPanelDialog.setPreferredSize(new java.awt.Dimension(100, 350));
      jPanelDialog.setLayout(new java.awt.BorderLayout(5, 5));

      jTextArea1.setEditable(false);
      jTextArea1.setColumns(20);
      jTextArea1.setLineWrap(true);
      jTextArea1.setRows(5);
      jTextArea1.setWrapStyleWord(true);
      jScrollPaneStatus.setViewportView(jTextArea1);

      jPanelDialog.add(jScrollPaneStatus, java.awt.BorderLayout.CENTER);

      jProgressBar.setString("Installation in progess ... Please be patient ...");
      jProgressBar.setStringPainted(true);
      jPanelDialog.add(jProgressBar, java.awt.BorderLayout.PAGE_START);

      jButtonAbort.setText("Abort");
      jButtonAbort.addComponentListener(new java.awt.event.ComponentAdapter()
      {
         public void componentShown(java.awt.event.ComponentEvent evt)
         {
            jButtonAbortComponentShown(evt);
         }
      });
      jButtonAbort.addActionListener(new java.awt.event.ActionListener()
      {
         public void actionPerformed(java.awt.event.ActionEvent evt)
         {
            jButtonAbortActionPerformed(evt);
         }
      });
      jPanelDialog.add(jButtonAbort, java.awt.BorderLayout.PAGE_END);

      getContentPane().add(jPanelDialog, java.awt.BorderLayout.CENTER);

      pack();
   }// </editor-fold>//GEN-END:initComponents

    private void jButtonAbortComponentShown(java.awt.event.ComponentEvent evt) {//GEN-FIRST:event_jButtonAbortComponentShown
}//GEN-LAST:event_jButtonAbortComponentShown

    private void jButtonAbortActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jButtonAbortActionPerformed
       if (readyToClose)
       {
          this.dispose();
       }
       else
       {
          this.managementThread.shouldStop();
          setFinished();
       }
}//GEN-LAST:event_jButtonAbortActionPerformed
   // Variables declaration - do not modify//GEN-BEGIN:variables
   private javax.swing.JButton jButtonAbort;
   private javax.swing.JPanel jPanelDialog;
   private javax.swing.JProgressBar jProgressBar;
   private javax.swing.JScrollPane jScrollPaneStatus;
   private javax.swing.JTextArea jTextArea1;
   // End of variables declaration//GEN-END:variables
}
