package com.beegfs.admon.gui.components.panels;


import com.beegfs.admon.gui.common.XMLParser;
import com.beegfs.admon.gui.common.enums.NodeTypesEnum;
import com.beegfs.admon.gui.common.exceptions.CommunicationException;
import com.beegfs.admon.gui.common.nodes.Node;
import com.beegfs.admon.gui.common.nodes.TypedNodes;
import static com.beegfs.admon.gui.common.tools.DefinesTk.DEFAULT_ENCODING_UTF8;
import com.beegfs.admon.gui.components.internalframes.management.JInternalFrameRemoteLogFiles;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;



public class RemoteLogFileTabPanel extends javax.swing.JPanel
{
   static final Logger LOGGER = Logger.getLogger(
      JInternalFrameRemoteLogFiles.class.getCanonicalName());
   private static final long serialVersionUID = 1L;
   private static final String THREAD_NAME = "LoadRemoteLogs";

   private final NodeTypesEnum type;

   private final String baseUrl;

   /**
    * Creates new form RemoteLogFilePanel
    */
   public RemoteLogFileTabPanel()
   {
      initComponents();
      type = NodeTypesEnum.STORAGE;
      baseUrl = "";
      initComboBox(new TypedNodes(NodeTypesEnum.STORAGE));
   }

   /**
    * Creates new form RemoteLogFilePanel
    */
   public RemoteLogFileTabPanel(TypedNodes nodes,
      String getLogfileUrl)
   {
      initComponents();
      type = nodes.getNodeType();
      baseUrl = getLogfileUrl;
      initComboBox(nodes);
   }

   private void initComboBox(TypedNodes nodes)
   {
      synchronized(nodes)
      {
         for (Node node : nodes)
         {
            jComboBoxNodes.addItem(node.getTypedNodeID());
         }
      }
      this.revalidate();
   }

   /**
    * This method is called from within the constructor to initialize the form. WARNING: Do NOT
    * modify this code. The content of this method is always regenerated by the Form Editor.
    */
   @SuppressWarnings("unchecked")
   // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
   private void initComponents()
   {
      java.awt.GridBagConstraints gridBagConstraints;

      jPanelControll = new javax.swing.JPanel();
      jButtonFatch = new javax.swing.JButton();
      jComboBoxNodes = new javax.swing.JComboBox<>();
      jButtonSave = new javax.swing.JButton();
      jLabelRestrict = new javax.swing.JLabel();
      jTextFieldLines = new javax.swing.JTextField();
      jLabelLines = new javax.swing.JLabel();
      jLabelNode = new javax.swing.JLabel();
      jScrollPaneContent = new javax.swing.JScrollPane();
      jTextPaneContent = new javax.swing.JTextPane();

      FormListener formListener = new FormListener();

      setLayout(new java.awt.BorderLayout());

      jPanelControll.setLayout(new java.awt.GridBagLayout());

      jButtonFatch.setText("Fetch");
      jButtonFatch.setMaximumSize(new java.awt.Dimension(100, 30));
      jButtonFatch.setMinimumSize(new java.awt.Dimension(100, 30));
      jButtonFatch.setPreferredSize(new java.awt.Dimension(100, 30));
      jButtonFatch.addActionListener(formListener);
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 5;
      gridBagConstraints.gridy = 0;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.NORTHWEST;
      gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 10);
      jPanelControll.add(jButtonFatch, gridBagConstraints);

      jComboBoxNodes.setMinimumSize(new java.awt.Dimension(150, 28));
      jComboBoxNodes.setPreferredSize(new java.awt.Dimension(150, 28));
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 1;
      gridBagConstraints.gridy = 0;
      gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
      gridBagConstraints.weightx = 1.0;
      gridBagConstraints.insets = new java.awt.Insets(5, 1, 5, 10);
      jPanelControll.add(jComboBoxNodes, gridBagConstraints);

      jButtonSave.setText("Save to file");
      jButtonSave.setMaximumSize(new java.awt.Dimension(100, 30));
      jButtonSave.setMinimumSize(new java.awt.Dimension(100, 30));
      jButtonSave.setPreferredSize(new java.awt.Dimension(100, 30));
      jButtonSave.addActionListener(formListener);
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 6;
      gridBagConstraints.gridy = 0;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.NORTHWEST;
      gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 10);
      jPanelControll.add(jButtonSave, gridBagConstraints);

      jLabelRestrict.setText("Restrict to the last");
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 2;
      gridBagConstraints.gridy = 0;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.EAST;
      gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 1);
      jPanelControll.add(jLabelRestrict, gridBagConstraints);

      jTextFieldLines.setHorizontalAlignment(javax.swing.JTextField.TRAILING);
      jTextFieldLines.setText("0");
      jTextFieldLines.setMinimumSize(new java.awt.Dimension(50, 28));
      jTextFieldLines.setPreferredSize(new java.awt.Dimension(50, 28));
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 3;
      gridBagConstraints.gridy = 0;
      gridBagConstraints.insets = new java.awt.Insets(5, 1, 5, 1);
      jPanelControll.add(jTextFieldLines, gridBagConstraints);

      jLabelLines.setText("lines (0 for all lines)");
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 4;
      gridBagConstraints.gridy = 0;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
      gridBagConstraints.insets = new java.awt.Insets(5, 1, 5, 10);
      jPanelControll.add(jLabelLines, gridBagConstraints);

      jLabelNode.setText("Node :");
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 0;
      gridBagConstraints.gridy = 0;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.EAST;
      gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 1);
      jPanelControll.add(jLabelNode, gridBagConstraints);

      add(jPanelControll, java.awt.BorderLayout.NORTH);

      jScrollPaneContent.setViewportView(jTextPaneContent);

      add(jScrollPaneContent, java.awt.BorderLayout.CENTER);
   }

   // Code for dispatching events from components to event handlers.

   private class FormListener implements java.awt.event.ActionListener
   {
      FormListener() {}
      public void actionPerformed(java.awt.event.ActionEvent evt)
      {
         if (evt.getSource() == jButtonFatch)
         {
            RemoteLogFileTabPanel.this.jButtonFatchActionPerformed(evt);
         }
         else if (evt.getSource() == jButtonSave)
         {
            RemoteLogFileTabPanel.this.jButtonSaveActionPerformed(evt);
         }
      }
   }// </editor-fold>//GEN-END:initComponents

   private void jButtonFatchActionPerformed(java.awt.event.ActionEvent evt)//GEN-FIRST:event_jButtonFatchActionPerformed
   {//GEN-HEADEREND:event_jButtonFatchActionPerformed
      int lines = 0;
      if (!jTextFieldLines.getText().trim().isEmpty())
      {
         try
         {
            lines = Integer.parseInt(jTextFieldLines.getText());
         }
         catch (NumberFormatException e)
         {
            lines = 0;
         }
      }
      
      String node = (String) jComboBoxNodes.getSelectedItem();
      String logFile = getFileContents(type.shortType(), node, lines);
      jTextPaneContent.setText(logFile);
   }//GEN-LAST:event_jButtonFatchActionPerformed

   private void jButtonSaveActionPerformed(java.awt.event.ActionEvent evt)//GEN-FIRST:event_jButtonSaveActionPerformed
   {//GEN-HEADEREND:event_jButtonSaveActionPerformed
      saveToFile();
   }//GEN-LAST:event_jButtonSaveActionPerformed

   public String getFileContents(String service, String typedNodeID, int lines)
   {
      String logFile = "";
      String node = Node.getNodeIDFromTypedNodeID(typedNodeID);

      try
      {
         int nodeNumID = Node.getNodeNumIDFromTypedNodeID(typedNodeID);

         String url = baseUrl + "?service=" + service + "&node=" + node + "&nodeNumID=" +
            String.valueOf(nodeNumID) + "&lines=" + lines;
         XMLParser parser = new XMLParser(url, THREAD_NAME);
         parser.update();
         logFile = parser.getValue("log");
      }
      catch (NumberFormatException ex)
      {
         LOGGER.log(Level.SEVERE, "Couldn't parse NodeNumID from string: {0}", typedNodeID);
      }
      catch (CommunicationException e)
      {
         LOGGER.log(Level.SEVERE, "Communication error occured", new Object[]{e, true});
      }
      return logFile;
   }

   public void saveToFile()
   {
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
               if (JOptionPane.showConfirmDialog(null, "The file " + f.getName() +
                       " exists. Do you really want to overwrite it?", "File exists",
                       JOptionPane.YES_NO_OPTION) == JOptionPane.OK_OPTION)
               {
                  if (f.canWrite())
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
                  bw.write(jTextPaneContent.getText());
                  bw.close();
               }
               catch (IOException e)
               {
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
   }

   // Variables declaration - do not modify//GEN-BEGIN:variables
   private javax.swing.JButton jButtonFatch;
   private javax.swing.JButton jButtonSave;
   private javax.swing.JComboBox<String> jComboBoxNodes;
   private javax.swing.JLabel jLabelLines;
   private javax.swing.JLabel jLabelNode;
   private javax.swing.JLabel jLabelRestrict;
   private javax.swing.JPanel jPanelControll;
   private javax.swing.JScrollPane jScrollPaneContent;
   private javax.swing.JTextField jTextFieldLines;
   private javax.swing.JTextPane jTextPaneContent;
   // End of variables declaration//GEN-END:variables
}
