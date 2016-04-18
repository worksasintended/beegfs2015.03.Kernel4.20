package com.beegfs.admon.gui.components.frames;

import com.beegfs.admon.gui.common.enums.UpdateDataTypeEnum;
import static com.beegfs.admon.gui.common.tools.DefinesTk.DEFAULT_ENCODING_UTF8;
import com.beegfs.admon.gui.components.internalframes.JInternalFrameUpdateableInterface;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.JOptionPane;

public class JFrameFileChooser extends javax.swing.JFrame
{
   static final Logger logger = Logger.getLogger(Logger.class.getCanonicalName());
   private static final long serialVersionUID = 1L;

   private final static int INITIAL_CAPACITY = 10;

   private final JInternalFrameUpdateableInterface parent;
   private final UpdateDataTypeEnum type;

   /**
    * Creates a FileChooser, which stores all lines of the file in the given outList
    *
    * @param outList The lines from the file are stored in this outList.
    * @param type The type of the file chooser, required for description label and title
    */
   public JFrameFileChooser(JInternalFrameUpdateableInterface parent,
      UpdateDataTypeEnum chooserType)
   {
      this.parent = parent;
      this.type = chooserType;

      initComponents();
      setTitle(type.getFileChooserTitle() );
      jTextAreaDescription.setText(type.getDescription());
   }

   /**
    * This method is called from within the constructor to initialize the form. WARNING: Do NOT
    * modify this code. The content of this method is always regenerated by the Form Editor.
    */
   @SuppressWarnings("unchecked")
   // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
   private void initComponents()
   {

      jPanelFrame = new javax.swing.JPanel();
      jPanelButtons = new javax.swing.JPanel();
      jButtonCancel = new javax.swing.JButton();
      jButtonLoad = new javax.swing.JButton();
      jFileChooser = new javax.swing.JFileChooser();
      jScrollPaneDescription = new javax.swing.JScrollPane();
      jTextAreaDescription = new javax.swing.JTextArea();

      setDefaultCloseOperation(javax.swing.WindowConstants.EXIT_ON_CLOSE);
      setResizable(false);

      jPanelFrame.setBorder(javax.swing.BorderFactory.createEmptyBorder(10, 10, 10, 10));
      jPanelFrame.setLayout(new java.awt.BorderLayout(5, 5));

      jPanelButtons.setBorder(javax.swing.BorderFactory.createEmptyBorder(0, 0, 10, 0));
      jPanelButtons.setPreferredSize(new java.awt.Dimension(504, 30));
      jPanelButtons.setLayout(new java.awt.FlowLayout(java.awt.FlowLayout.CENTER, 100, 0));

      jButtonCancel.setText("Cancel");
      jButtonCancel.setPreferredSize(new java.awt.Dimension(80, 30));
      jButtonCancel.addActionListener(new java.awt.event.ActionListener()
      {
         public void actionPerformed(java.awt.event.ActionEvent evt)
         {
            jButtonCancelActionPerformed(evt);
         }
      });
      jPanelButtons.add(jButtonCancel);

      jButtonLoad.setText("Load");
      jButtonLoad.setPreferredSize(new java.awt.Dimension(80, 30));
      jButtonLoad.addActionListener(new java.awt.event.ActionListener()
      {
         public void actionPerformed(java.awt.event.ActionEvent evt)
         {
            jButtonLoadActionPerformed(evt);
         }
      });
      jPanelButtons.add(jButtonLoad);

      jPanelFrame.add(jPanelButtons, java.awt.BorderLayout.SOUTH);

      jFileChooser.setControlButtonsAreShown(false);
      jFileChooser.addActionListener(new java.awt.event.ActionListener()
      {
         public void actionPerformed(java.awt.event.ActionEvent evt)
         {
            jFileChooserActionPerformed(evt);
         }
      });
      jPanelFrame.add(jFileChooser, java.awt.BorderLayout.CENTER);

      jScrollPaneDescription.setBorder(null);
      jScrollPaneDescription.setHorizontalScrollBarPolicy(javax.swing.ScrollPaneConstants.HORIZONTAL_SCROLLBAR_NEVER);
      jScrollPaneDescription.setVerticalScrollBarPolicy(javax.swing.ScrollPaneConstants.VERTICAL_SCROLLBAR_NEVER);

      jTextAreaDescription.setEditable(false);
      jTextAreaDescription.setBackground(new java.awt.Color(238, 238, 238));
      jTextAreaDescription.setColumns(20);
      jTextAreaDescription.setFont(new java.awt.Font("Dialog", 1, 14)); // NOI18N
      jTextAreaDescription.setLineWrap(true);
      jTextAreaDescription.setRows(3);
      jTextAreaDescription.setText("Please choose the file to import.\n\nThe file must contain one host per line.");
      jTextAreaDescription.setWrapStyleWord(true);
      jTextAreaDescription.setBorder(null);
      jScrollPaneDescription.setViewportView(jTextAreaDescription);

      jPanelFrame.add(jScrollPaneDescription, java.awt.BorderLayout.NORTH);

      getContentPane().add(jPanelFrame, java.awt.BorderLayout.CENTER);

      pack();
   }// </editor-fold>//GEN-END:initComponents

    private void jButtonLoadActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jButtonLoadActionPerformed
      File file = null;
      BufferedReader in = null;
      ArrayList<String> list = new ArrayList<>(INITIAL_CAPACITY);

      try
      {
         file = jFileChooser.getSelectedFile();
      }
      catch (java.lang.NullPointerException e)
      {
         logger.log(Level.FINEST, "Internal error", e);
      }

      if (file != null && file.exists() && file.isFile() && file.canRead())
      {
         try
         {
            in = new BufferedReader(new InputStreamReader(new FileInputStream(file),
               DEFAULT_ENCODING_UTF8));
            String line = in.readLine();
            while (line != null)
            {
               line = line.trim();
               if ((!line.isEmpty()) && (line.indexOf(' ') == -1))
               {
                  list.add(line);
               }
               line = in.readLine();
            }
            this.dispose();
         }
         catch (FileNotFoundException e)
         {
            logger.log(Level.WARNING, "File not found", e);
            JOptionPane.showMessageDialog(null, "The selected file does not exist or is not " +
               "readable!", "Error", JOptionPane.ERROR_MESSAGE);
         }
         catch (IOException e)
         {
            logger.log(Level.WARNING, "IO error", e);
            JOptionPane.showMessageDialog(null, "The selected file does not exist or is not " +
               "readable!", "Error", JOptionPane.ERROR_MESSAGE);
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
            catch (IOException e)
            {
               logger.log(Level.FINEST, "Could not close file.", e);
            }
         }
      }
      else
      {
         JOptionPane.showMessageDialog(null, "The selected file does not exist or is not " +
            "readable!", "Error", JOptionPane.ERROR_MESSAGE);
      }

      parent.updateData(list, type);
}//GEN-LAST:event_jButtonLoadActionPerformed

    private void jButtonCancelActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jButtonCancelActionPerformed
      this.dispose();
}//GEN-LAST:event_jButtonCancelActionPerformed

    private void jFileChooserActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jFileChooserActionPerformed
      jButtonLoadActionPerformed(evt);
    }//GEN-LAST:event_jFileChooserActionPerformed
   // Variables declaration - do not modify//GEN-BEGIN:variables
   private javax.swing.JButton jButtonCancel;
   private javax.swing.JButton jButtonLoad;
   private javax.swing.JFileChooser jFileChooser;
   private javax.swing.JPanel jPanelButtons;
   private javax.swing.JPanel jPanelFrame;
   private javax.swing.JScrollPane jScrollPaneDescription;
   private javax.swing.JTextArea jTextAreaDescription;
   // End of variables declaration//GEN-END:variables
}
