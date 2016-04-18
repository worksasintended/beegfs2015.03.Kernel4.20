package com.beegfs.admon.gui.components.internalframes.management;

import com.beegfs.admon.gui.common.XMLParser;
import com.beegfs.admon.gui.common.exceptions.CommunicationException;
import com.beegfs.admon.gui.common.tools.CryptTk;
import com.beegfs.admon.gui.common.tools.GuiTk;
import com.beegfs.admon.gui.common.tools.HttpTk;
import com.beegfs.admon.gui.components.internalframes.JInternalFrameInterface;
import com.beegfs.admon.gui.components.managers.FrameManager;
import com.beegfs.admon.gui.program.Main;
import java.util.Arrays;
import java.util.TreeMap;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.JOptionPane;


public class JInternalFrameUserSettings extends javax.swing.JInternalFrame 
   implements JInternalFrameInterface
{
   static final Logger LOGGER = Logger.getLogger(
           JInternalFrameUserSettings.class.getCanonicalName());
   private static final long serialVersionUID = 1L;
   private static final String THREAD_NAME = "UserSettings";

    /** Creates new form JInternalFrameMetaNode */
    public JInternalFrameUserSettings() {
        initComponents();
        setTitle(getFrameTitle());
        setFrameIcon(GuiTk.getFrameIcon());
        if (Main.getSession().getInfoAutologinDisabled()) {
            jButtonAutologinInfo.setText("Enable passwordless autologin for \"Information\" user");
        }
        else {
            jButtonAutologinInfo.setText("Disable passwordless autologin for \"Information\" user");
        }
  }

   @Override
   public boolean isEqual(JInternalFrameInterface obj)
   {
      return obj instanceof JInternalFrameUserSettings;
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
      java.awt.GridBagConstraints gridBagConstraints;

      jScrollPaneFrame = new javax.swing.JScrollPane();
      jPanelFrame = new javax.swing.JPanel();
      jPanelPasswords = new javax.swing.JPanel();
      jPanel2 = new javax.swing.JPanel();
      jLabelInfoAdminPW = new javax.swing.JLabel();
      jPasswordFieldInfoAdminPW = new javax.swing.JPasswordField();
      jPasswordFieldInfoNewPW = new javax.swing.JPasswordField();
      jLabelInfoNewPW = new javax.swing.JLabel();
      jPasswordFieldInfoNewPWConfirm = new javax.swing.JPasswordField();
      jLabelInfoNewPWConfirm = new javax.swing.JLabel();
      jButtonChangeInformation = new javax.swing.JButton();
      filler1 = new javax.swing.Box.Filler(new java.awt.Dimension(5, 5), new java.awt.Dimension(5, 5), new java.awt.Dimension(5, 5));
      jPanel3 = new javax.swing.JPanel();
      jLabelAdminOldPW = new javax.swing.JLabel();
      jPasswordFieldAdminOldPW = new javax.swing.JPasswordField();
      jPasswordFieldAdminNewPW = new javax.swing.JPasswordField();
      jLabelAdminNewPW = new javax.swing.JLabel();
      jPasswordFieldAdminNewPWConfirm = new javax.swing.JPasswordField();
      jLabelAdminNewPWConfirm = new javax.swing.JLabel();
      jButtonChangeAdmin = new javax.swing.JButton();
      jPanelAutologin = new javax.swing.JPanel();
      jLabelAutologinInfo = new javax.swing.JLabel();
      jPasswordFieldIAutologinInfo = new javax.swing.JPasswordField();
      jButtonAutologinInfo = new javax.swing.JButton();

      setClosable(true);
      setResizable(true);
      setPreferredSize(new java.awt.Dimension(970, 331));
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
      jPanelFrame.setPreferredSize(new java.awt.Dimension(936, 273));
      jPanelFrame.setLayout(new java.awt.BorderLayout(5, 5));

      jPanelPasswords.setBorder(javax.swing.BorderFactory.createTitledBorder("Change Passwords"));
      jPanelPasswords.setPreferredSize(new java.awt.Dimension(936, 174));
      jPanelPasswords.setLayout(new javax.swing.BoxLayout(jPanelPasswords, javax.swing.BoxLayout.LINE_AXIS));

      jPanel2.setBorder(javax.swing.BorderFactory.createTitledBorder("Information user"));
      jPanel2.setMinimumSize(new java.awt.Dimension(450, 200));
      jPanel2.setPreferredSize(new java.awt.Dimension(450, 200));
      jPanel2.setLayout(new java.awt.GridBagLayout());

      jLabelInfoAdminPW.setHorizontalAlignment(javax.swing.SwingConstants.RIGHT);
      jLabelInfoAdminPW.setText("Current \"Administrator\" password : ");
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 0;
      gridBagConstraints.gridy = 0;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.EAST;
      gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
      jPanel2.add(jLabelInfoAdminPW, gridBagConstraints);
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 1;
      gridBagConstraints.gridy = 0;
      gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
      gridBagConstraints.weightx = 1.0;
      gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
      jPanel2.add(jPasswordFieldInfoAdminPW, gridBagConstraints);
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 1;
      gridBagConstraints.gridy = 1;
      gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
      gridBagConstraints.weightx = 1.0;
      gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
      jPanel2.add(jPasswordFieldInfoNewPW, gridBagConstraints);

      jLabelInfoNewPW.setHorizontalAlignment(javax.swing.SwingConstants.RIGHT);
      jLabelInfoNewPW.setText("New \"Information\" password : ");
      jLabelInfoNewPW.setMaximumSize(new java.awt.Dimension(204, 15));
      jLabelInfoNewPW.setMinimumSize(new java.awt.Dimension(204, 15));
      jLabelInfoNewPW.setPreferredSize(new java.awt.Dimension(204, 15));
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 0;
      gridBagConstraints.gridy = 1;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.EAST;
      gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
      jPanel2.add(jLabelInfoNewPW, gridBagConstraints);
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 1;
      gridBagConstraints.gridy = 2;
      gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
      gridBagConstraints.weightx = 1.0;
      gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
      jPanel2.add(jPasswordFieldInfoNewPWConfirm, gridBagConstraints);

      jLabelInfoNewPWConfirm.setHorizontalAlignment(javax.swing.SwingConstants.RIGHT);
      jLabelInfoNewPWConfirm.setText("Confirm New \"Information\" password : ");
      jLabelInfoNewPWConfirm.setHorizontalTextPosition(javax.swing.SwingConstants.RIGHT);
      jLabelInfoNewPWConfirm.setMaximumSize(new java.awt.Dimension(259, 15));
      jLabelInfoNewPWConfirm.setMinimumSize(new java.awt.Dimension(259, 15));
      jLabelInfoNewPWConfirm.setPreferredSize(new java.awt.Dimension(259, 15));
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 0;
      gridBagConstraints.gridy = 2;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.EAST;
      gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
      jPanel2.add(jLabelInfoNewPWConfirm, gridBagConstraints);

      jButtonChangeInformation.setText("Change");
      jButtonChangeInformation.addActionListener(new java.awt.event.ActionListener()
      {
         public void actionPerformed(java.awt.event.ActionEvent evt)
         {
            jButtonChangeInformationActionPerformed(evt);
         }
      });
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 1;
      gridBagConstraints.gridy = 3;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
      gridBagConstraints.insets = new java.awt.Insets(10, 10, 10, 10);
      jPanel2.add(jButtonChangeInformation, gridBagConstraints);

      jPanelPasswords.add(jPanel2);
      jPanelPasswords.add(filler1);

      jPanel3.setBorder(javax.swing.BorderFactory.createTitledBorder("Administration user"));
      jPanel3.setMinimumSize(new java.awt.Dimension(450, 200));
      jPanel3.setPreferredSize(new java.awt.Dimension(450, 200));
      jPanel3.setLayout(new java.awt.GridBagLayout());

      jLabelAdminOldPW.setHorizontalAlignment(javax.swing.SwingConstants.RIGHT);
      jLabelAdminOldPW.setText("Current \"Administrator\" password : ");
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 0;
      gridBagConstraints.gridy = 0;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.EAST;
      gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
      jPanel3.add(jLabelAdminOldPW, gridBagConstraints);
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 1;
      gridBagConstraints.gridy = 0;
      gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
      gridBagConstraints.weightx = 1.0;
      gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
      jPanel3.add(jPasswordFieldAdminOldPW, gridBagConstraints);
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 1;
      gridBagConstraints.gridy = 1;
      gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
      gridBagConstraints.weightx = 1.0;
      gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
      jPanel3.add(jPasswordFieldAdminNewPW, gridBagConstraints);

      jLabelAdminNewPW.setHorizontalAlignment(javax.swing.SwingConstants.RIGHT);
      jLabelAdminNewPW.setText("New \"Administrator\" password : ");
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 0;
      gridBagConstraints.gridy = 1;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.EAST;
      gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
      jPanel3.add(jLabelAdminNewPW, gridBagConstraints);
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 1;
      gridBagConstraints.gridy = 2;
      gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
      gridBagConstraints.weightx = 1.0;
      gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
      jPanel3.add(jPasswordFieldAdminNewPWConfirm, gridBagConstraints);

      jLabelAdminNewPWConfirm.setHorizontalAlignment(javax.swing.SwingConstants.RIGHT);
      jLabelAdminNewPWConfirm.setText("Confirm New \"Administrator\" password : ");
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 0;
      gridBagConstraints.gridy = 2;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.EAST;
      gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
      jPanel3.add(jLabelAdminNewPWConfirm, gridBagConstraints);

      jButtonChangeAdmin.setText("Change");
      jButtonChangeAdmin.addActionListener(new java.awt.event.ActionListener()
      {
         public void actionPerformed(java.awt.event.ActionEvent evt)
         {
            jButtonChangeAdminActionPerformed(evt);
         }
      });
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 1;
      gridBagConstraints.gridy = 3;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
      gridBagConstraints.insets = new java.awt.Insets(10, 10, 10, 10);
      jPanel3.add(jButtonChangeAdmin, gridBagConstraints);

      jPanelPasswords.add(jPanel3);

      jPanelFrame.add(jPanelPasswords, java.awt.BorderLayout.CENTER);

      jPanelAutologin.setBorder(javax.swing.BorderFactory.createTitledBorder("Passwordless autologin for \"Information\" user"));
      jPanelAutologin.setLayout(new java.awt.GridBagLayout());

      jLabelAutologinInfo.setText("Current \"Administrator\" password : ");
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 0;
      gridBagConstraints.gridy = 0;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.EAST;
      gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
      jPanelAutologin.add(jLabelAutologinInfo, gridBagConstraints);
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 1;
      gridBagConstraints.gridy = 0;
      gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
      gridBagConstraints.weightx = 1.0;
      gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
      jPanelAutologin.add(jPasswordFieldIAutologinInfo, gridBagConstraints);

      jButtonAutologinInfo.setText("Submit");
      jButtonAutologinInfo.addActionListener(new java.awt.event.ActionListener()
      {
         public void actionPerformed(java.awt.event.ActionEvent evt)
         {
            jButtonAutologinInfoActionPerformed(evt);
         }
      });
      gridBagConstraints = new java.awt.GridBagConstraints();
      gridBagConstraints.gridx = 2;
      gridBagConstraints.gridy = 0;
      gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
      gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
      jPanelAutologin.add(jButtonAutologinInfo, gridBagConstraints);

      jPanelFrame.add(jPanelAutologin, java.awt.BorderLayout.SOUTH);

      jScrollPaneFrame.setViewportView(jPanelFrame);

      javax.swing.GroupLayout layout = new javax.swing.GroupLayout(getContentPane());
      getContentPane().setLayout(layout);
      layout.setHorizontalGroup(
         layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
         .addComponent(jScrollPaneFrame, javax.swing.GroupLayout.DEFAULT_SIZE, 959, Short.MAX_VALUE)
      );
      layout.setVerticalGroup(
         layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
         .addComponent(jScrollPaneFrame, javax.swing.GroupLayout.DEFAULT_SIZE, 301, Short.MAX_VALUE)
      );

      pack();
   }// </editor-fold>//GEN-END:initComponents

    private void formInternalFrameClosed(javax.swing.event.InternalFrameEvent evt) {//GEN-FIRST:event_formInternalFrameClosed
        FrameManager.delFrame(this);
    }//GEN-LAST:event_formInternalFrameClosed

    private void jButtonChangeInformationActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jButtonChangeInformationActionPerformed
        //first check if new PWs equal, otherwise we can stop right now
        if (Arrays.equals(jPasswordFieldInfoNewPW.getPassword(),jPasswordFieldInfoNewPWConfirm.getPassword())) {
            try {
                // get nonce to hash admin password and do the operation with the hashed pw
                XMLParser parser = new XMLParser(HttpTk.generateAdmonUrl("/XML_GetNonce"),
                   THREAD_NAME);
                parser.update();
                TreeMap<String, String> data = parser.getTreeMap();
                int nonceID = Integer.parseInt(data.get("id"));
                long nonce = Long.parseLong(data.get("nonce"));
                String pw = CryptTk.getMD5(jPasswordFieldInfoAdminPW.getPassword());
                String secret = CryptTk.cryptWithNonce(pw, nonce);
                String newPw = CryptTk.getMD5(jPasswordFieldInfoNewPW.getPassword());
                String url = HttpTk.generateAdmonUrl("/XML_ChangePW");
                String params = "?user=information&newPw="+newPw+"&secret="+secret+"&nonceID="+nonceID;
                parser.setUrl(url+params);
                parser.update();
                boolean success = Boolean.parseBoolean(parser.getValue("success"));
                if (success) {
                    JOptionPane.showMessageDialog(null, "The password was changed.", "Password changed", JOptionPane.INFORMATION_MESSAGE);
                }
                else {
                    JOptionPane.showMessageDialog(null, "The password could not be changed. Please make sure that you supplied the right Administrator password", "Password unchanged", JOptionPane.ERROR_MESSAGE);
                }
            } catch (CommunicationException e) {
                LOGGER.log(Level.SEVERE, "Communication Error occured", new Object[]{e, true});
            }
        }
        else {
          JOptionPane.showMessageDialog(null, "New passwords do not match", "New passwords do not match", JOptionPane.ERROR_MESSAGE);
        }
    }//GEN-LAST:event_jButtonChangeInformationActionPerformed

    private void jButtonChangeAdminActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jButtonChangeAdminActionPerformed
        //first check if new PWs equal, otherwise we can stop right now
        if (Arrays.equals(jPasswordFieldAdminNewPW.getPassword(),jPasswordFieldAdminNewPWConfirm.getPassword())) {
            try {
                // get nonce to hash admin password and do the operation with the hashed pw
                XMLParser parser = new XMLParser(HttpTk.generateAdmonUrl("/XML_GetNonce"),
                   THREAD_NAME);
                parser.update();
                TreeMap<String, String> data = parser.getTreeMap();
                int nonceID = Integer.parseInt(data.get("id"));
                long nonce = Long.parseLong(data.get("nonce"));
                String pw = CryptTk.getMD5(jPasswordFieldAdminOldPW.getPassword());
                String secret = CryptTk.cryptWithNonce(pw, nonce);
                String newPw = CryptTk.getMD5(jPasswordFieldAdminNewPW.getPassword());
                String url = HttpTk.generateAdmonUrl("/XML_ChangePW");
                String params = "?user=admin&newPw="+newPw+"&secret="+secret+"&nonceID="+nonceID;
                parser.setUrl(url+params);
                parser.update();
                boolean success = Boolean.parseBoolean(parser.getValue("success"));
                if (success) {
                    JOptionPane.showMessageDialog(null, "The password was changed.", "Password changed", JOptionPane.INFORMATION_MESSAGE);
                }
                else {
                    JOptionPane.showMessageDialog(null, "The password could not be changed. Please make sure that you supplied the right Administrator password", "Password unchanged", JOptionPane.ERROR_MESSAGE);
                }
            } catch (CommunicationException e) {
                LOGGER.log(Level.SEVERE, "Communication Error occured", new Object[]{e, true});
            }
        }
        else {
          JOptionPane.showMessageDialog(null, "New passwords do not match", "New passwords do not match", JOptionPane.ERROR_MESSAGE);
        }
    }//GEN-LAST:event_jButtonChangeAdminActionPerformed

    private void jButtonAutologinInfoActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jButtonAutologinInfoActionPerformed
           try {
                // get nonce to hash admin password and do the operation with the hashed pw
                XMLParser parser = new XMLParser(HttpTk.generateAdmonUrl("/XML_GetNonce"),
                   THREAD_NAME);
                parser.update();
                TreeMap<String, String> data = parser.getTreeMap();
                int nonceID = Integer.parseInt(data.get("id"));
                long nonce = Long.parseLong(data.get("nonce"));
                String pw = CryptTk.getMD5(jPasswordFieldIAutologinInfo.getPassword());
                String secret = CryptTk.cryptWithNonce(pw, nonce);
                String url = HttpTk.generateAdmonUrl("/XML_ChangePW");
                String params;
                if (Main.getSession().getInfoAutologinDisabled()) {
                   params = "?action=enableInfo&secret="+secret+"&nonceID="+nonceID;
                }
                else {
                   params = "?action=disableInfo&secret="+secret+"&nonceID="+nonceID;
                }
                parser.setUrl(url+params);
                parser.update();
                boolean success = Boolean.parseBoolean(parser.getValue("success"));
                if (success) {
                    if (Main.getSession().getInfoAutologinDisabled()) {
                        JOptionPane.showMessageDialog(null, "Passwordless autologin for Information user successfully enabled", "Operation performed", JOptionPane.INFORMATION_MESSAGE);
                        Main.getSession().setInfoAutologinDisabled(false);
                        jButtonAutologinInfo.setText("Disable passwordless autologin for \"Information\" user");
                    }
                    else {
                        JOptionPane.showMessageDialog(null, "Passwordless autologin for Information user successfully disabled", "Operation performed", JOptionPane.INFORMATION_MESSAGE);
                        Main.getSession().setInfoAutologinDisabled(true);
                        jButtonAutologinInfo.setText("Enable passwordless autologin for \"Information\" user");
                    }
                }
                else {
                    JOptionPane.showMessageDialog(null, "Could not perform operation. Please make sure that you supplied the right Administrator password", "Operation failed", JOptionPane.ERROR_MESSAGE);
                }
            } catch (CommunicationException e) {
                LOGGER.log(Level.SEVERE, "Communication Error occured", new Object[]{e, true});
            }
    }//GEN-LAST:event_jButtonAutologinInfoActionPerformed


   // Variables declaration - do not modify//GEN-BEGIN:variables
   private javax.swing.Box.Filler filler1;
   private javax.swing.JButton jButtonAutologinInfo;
   private javax.swing.JButton jButtonChangeAdmin;
   private javax.swing.JButton jButtonChangeInformation;
   private javax.swing.JLabel jLabelAdminNewPW;
   private javax.swing.JLabel jLabelAdminNewPWConfirm;
   private javax.swing.JLabel jLabelAdminOldPW;
   private javax.swing.JLabel jLabelAutologinInfo;
   private javax.swing.JLabel jLabelInfoAdminPW;
   private javax.swing.JLabel jLabelInfoNewPW;
   private javax.swing.JLabel jLabelInfoNewPWConfirm;
   private javax.swing.JPanel jPanel2;
   private javax.swing.JPanel jPanel3;
   private javax.swing.JPanel jPanelAutologin;
   private javax.swing.JPanel jPanelFrame;
   private javax.swing.JPanel jPanelPasswords;
   private javax.swing.JPasswordField jPasswordFieldAdminNewPW;
   private javax.swing.JPasswordField jPasswordFieldAdminNewPWConfirm;
   private javax.swing.JPasswordField jPasswordFieldAdminOldPW;
   private javax.swing.JPasswordField jPasswordFieldIAutologinInfo;
   private javax.swing.JPasswordField jPasswordFieldInfoAdminPW;
   private javax.swing.JPasswordField jPasswordFieldInfoNewPW;
   private javax.swing.JPasswordField jPasswordFieldInfoNewPWConfirm;
   private javax.swing.JScrollPane jScrollPaneFrame;
   // End of variables declaration//GEN-END:variables

   @Override
    public final String getFrameTitle() {
        return "User Settings";
    }

}
