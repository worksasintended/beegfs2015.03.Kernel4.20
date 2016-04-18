package com.beegfs.admon.gui.components.menus;

import com.beegfs.admon.gui.common.Groups;
import com.beegfs.admon.gui.common.XMLParser;
import com.beegfs.admon.gui.common.enums.NodeTypesEnum;
import static com.beegfs.admon.gui.common.enums.StatsTypeEnum.STATS_CLIENT_METADATA;
import static com.beegfs.admon.gui.common.enums.StatsTypeEnum.STATS_CLIENT_STORAGE;
import static com.beegfs.admon.gui.common.enums.StatsTypeEnum.STATS_USER_METADATA;
import static com.beegfs.admon.gui.common.enums.StatsTypeEnum.STATS_USER_STORAGE;
import com.beegfs.admon.gui.common.exceptions.CommunicationException;
import com.beegfs.admon.gui.common.nodes.Node;
import com.beegfs.admon.gui.common.nodes.Nodes;
import com.beegfs.admon.gui.common.threading.GuiThread;
import com.beegfs.admon.gui.components.internalframes.JInternalFrameInterface;
import com.beegfs.admon.gui.components.internalframes.install.JInternalFrameInstall;
import com.beegfs.admon.gui.components.internalframes.install.JInternalFrameInstallationConfig;
import com.beegfs.admon.gui.components.internalframes.install.JInternalFrameUninstall;
import com.beegfs.admon.gui.components.internalframes.management.JInternalFrameFileBrowser;
import com.beegfs.admon.gui.components.internalframes.management.JInternalFrameKnownProblems;
import com.beegfs.admon.gui.components.internalframes.management.JInternalFrameLogFile;
import com.beegfs.admon.gui.components.internalframes.management.JInternalFrameRemoteLogFiles;
import com.beegfs.admon.gui.components.internalframes.management.JInternalFrameStartStop;
import com.beegfs.admon.gui.components.internalframes.management.JInternalFrameStriping;
import com.beegfs.admon.gui.components.internalframes.stats.JInternalFrameMetaNode;
import com.beegfs.admon.gui.components.internalframes.stats.JInternalFrameMetaNodesOverview;
import com.beegfs.admon.gui.components.internalframes.stats.JInternalFrameStats;
import com.beegfs.admon.gui.components.internalframes.stats.JInternalFrameStorageNode;
import com.beegfs.admon.gui.components.internalframes.stats.JInternalFrameStorageNodesOverview;
import com.beegfs.admon.gui.components.managers.FrameManager;
import com.beegfs.admon.gui.program.Main;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import static java.lang.Integer.parseInt;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.TreeMap;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.JInternalFrame;
import javax.swing.JTree;
import javax.swing.border.EtchedBorder;
import javax.swing.text.Position;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.MutableTreeNode;
import javax.swing.tree.TreeNode;
import javax.swing.tree.TreePath;

public class JTreeMenu extends JTree
{

   static final Logger logger = Logger.getLogger(
           JTreeMenu.class.getCanonicalName());
   private static final long serialVersionUID = 1L;

   private static final String THREAD_NAME = "MenuNodeList";

   private final transient XMLParser parserNodeList;

   private final transient MenuUpdateThread updateThread;

   public JTreeMenu(XMLParser nodeListParser)
   {
      TreeMenuCellRenderer menuCellRenderer = new TreeMenuCellRenderer();
      this.setCellRenderer(menuCellRenderer);
      this.setEnabled(true);
      this.setOpaque(true);
      this.setDoubleBuffered(true);
      this.setBorder(new EtchedBorder());

      this.addMouseListener(new TreeMenuMouseListener());

      DefaultTreeModel model = (DefaultTreeModel) this.getModel();
      DefaultMutableTreeNode topNode = new DefaultMutableTreeNode("Menu");
      model.setRoot(topNode);
      rebuildTree();

      parserNodeList = nodeListParser;
      updateThread = new MenuUpdateThread(THREAD_NAME);
   }

   final public boolean updateMetadataNodes()
   {
      try
      {
         Groups groups = new Groups();
         ArrayList<TreeMap<String, String>> groupedNodes =
            parserNodeList.getVectorOfAttributeTreeMaps("meta");
         for (TreeMap<String, String> groupedNode : groupedNodes)
         {
            String group = groupedNode.get("group");
            String nodeID = groupedNode.get("value");
            int nodeNumID = parseInt(groupedNode.get("nodeNumID"));
            Node node = new Node(nodeNumID, nodeID, group, NodeTypesEnum.METADATA);
            groups.getGroup(group).addNode(node);
         }

         DefaultTreeModel model = (DefaultTreeModel) this.getModel();
         TreePath parentPath = this.getNextMatch("Node details", 0, Position.Bias.Forward);

         DefaultMutableTreeNode node;

         String path = parentPath.getParentPath().getLastPathComponent().toString();
         if(path.equalsIgnoreCase("Metadata nodes") )
         {
            node = (DefaultMutableTreeNode) parentPath.getLastPathComponent();
         }
         else
         {
            parentPath = this.getNextMatch("Node details", 0, Position.Bias.Backward);
            node = (DefaultMutableTreeNode) parentPath.getLastPathComponent();
         }

         boolean isExpanded = this.isExpanded(parentPath);

         if (node.getChildCount() > 0)
         {
            for (int i = node.getChildCount() - 1; i >= 0; i--)
            {
               DefaultMutableTreeNode n = (DefaultMutableTreeNode) node.getChildAt(i);
               model.removeNodeFromParent(n);
            }
         }

         for (final String groupName : groups.getGroupNames())
         {
            Nodes nodes = groups.getGroup(groupName).getNodes();
            for (Iterator<Node> iter = nodes.iterator(); iter.hasNext();)
            {
               final Node tmpNode = iter.next();
               model.insertNodeInto(new DefaultMutableTreeNode(tmpNode.getTypedNodeID()),
                  (MutableTreeNode) parentPath.getLastPathComponent(),
                  ((TreeNode) parentPath.getLastPathComponent()).getChildCount());
            }
         }

         if (isExpanded)
         {
            this.expandPath(parentPath);
         }

         return true;
      }
      catch (CommunicationException e)
      {
         logger.log(Level.SEVERE, "Communication Error occured", new Object[]{e, true});
         return false;
      }
      catch (java.lang.NullPointerException e)
      {
         logger.log(Level.FINEST, "Internal error.", e);
         return false;
      }
   }

   final public boolean updateStorageNodes()
   {
      try
      {
         Groups groups = new Groups();
         ArrayList<TreeMap<String, String>> groupedNodes =
            parserNodeList.getVectorOfAttributeTreeMaps("storage");
         for (TreeMap<String, String> groupedNode : groupedNodes)
         {
            String group = groupedNode.get("group");
            String nodeID = groupedNode.get("value");
            int nodeNumID = parseInt(groupedNode.get("nodeNumID"));
            Node node = new Node(nodeNumID, nodeID, group, NodeTypesEnum.STORAGE);
            groups.getGroup(group).addNode(node);
         }
         DefaultTreeModel model = (DefaultTreeModel) this.getModel();
         TreePath parentPath = this.getNextMatch("Node details", 0, Position.Bias.Backward);

         DefaultMutableTreeNode node;

         String path = parentPath.getParentPath().getLastPathComponent().toString();
         if(path.equalsIgnoreCase("Storage nodes") )
         {
            node = (DefaultMutableTreeNode) parentPath.getLastPathComponent();
         }
         else
         {
            parentPath = this.getNextMatch("Node details", 0, Position.Bias.Forward);
            node = (DefaultMutableTreeNode) parentPath.getLastPathComponent();
         }

         boolean isExpanded = this.isExpanded(parentPath);

         if (node.getChildCount() > 0)
         {
            for (int i = node.getChildCount() - 1; i >= 0; i--)
            {
               DefaultMutableTreeNode n = (DefaultMutableTreeNode) node.getChildAt(i);
               model.removeNodeFromParent(n);
            }
         }

         for (final String groupName : groups.getGroupNames())
         {
            Nodes nodes = groups.getGroup(groupName).getNodes();
            for (Iterator<Node> iter = nodes.iterator(); iter.hasNext();)
            {
               final Node tmpNode = iter.next();
               model.insertNodeInto(new DefaultMutableTreeNode(tmpNode.getTypedNodeID()),
                  (MutableTreeNode) parentPath.getLastPathComponent(),
                  ((TreeNode) parentPath.getLastPathComponent()).getChildCount());
            }
         }

         if (isExpanded)
         {
            this.expandPath(parentPath);
         }
         
         return true;
      }
      catch (CommunicationException e)
      {
         logger.log(Level.SEVERE, "Communication Error occured", new Object[]{e, true});
         return false;
      }
      catch (java.lang.NullPointerException e)
      {
         logger.log(Level.FINEST, "Internal error.", e);
         return false;
      }
   }

   private boolean openFrame(TreePath path)
   {
      JInternalFrame frame;

      // make the right frame
      Object[] pathObjects = path.getPath();
      int pathCount = path.getPathCount();

      String submenu = pathObjects[1].toString();
      String item = pathObjects[pathCount - 1].toString();

      // First check the submenu we are in, especially if we are in meta or storage nodes
      if (submenu.equalsIgnoreCase("metadata nodes") && (Main.getSession().getIsInfo()))
      {
         if (item.equalsIgnoreCase("overview"))
         {
            frame = new JInternalFrameMetaNodesOverview();
         }
         else
         if (!item.equalsIgnoreCase("Node details"))
         {
            frame = new JInternalFrameMetaNode(new Node(item, NodeTypesEnum.METADATA));
         }
         else
         {
            return true;
         }
      }
      else if (submenu.equalsIgnoreCase("storage nodes") && (Main.getSession().getIsInfo()))
      {
         if (item.equalsIgnoreCase("overview"))
         {
            frame = new JInternalFrameStorageNodesOverview();
         }
         else
         if (!item.equalsIgnoreCase("Node details"))
         {
            frame = new JInternalFrameStorageNode(new Node(item, NodeTypesEnum.STORAGE));
         }
         else
         {
            return true;
         }
      }
      else if (submenu.equalsIgnoreCase("Client statistics") && (Main.getSession().getIsInfo()))
      {
         if (item.equalsIgnoreCase("metadata") )
         {
            frame = new JInternalFrameStats(STATS_CLIENT_METADATA);
         }
         else if (item.equalsIgnoreCase("storage") )
         {
            frame = new JInternalFrameStats(STATS_CLIENT_STORAGE);
         }
         else
         {
            return true;
         }
      }
      else if (submenu.equalsIgnoreCase("User statistics") && (Main.getSession().getIsInfo()))
      {
         if (item.equalsIgnoreCase("metadata") )
         {
            frame = new JInternalFrameStats(STATS_USER_METADATA);
         }
         else if (item.equalsIgnoreCase("storage") )
         {
            frame = new JInternalFrameStats(STATS_USER_STORAGE);
         }
         else
         {
            return true;
         }
      }
      /*
      else if (submenu.equalsIgnoreCase("Quota") && (Main.getSession().getIsInfo()))
      {
         if (item.equalsIgnoreCase("Show Quota") )
         {
            frame = new JInternalFrameShowQuota();
         }
         else if (item.equalsIgnoreCase("Set Quota Limits") )
         {
            frame = new JInternalFrameSetQuota();
         }
         else
         {
            return true;
         }
      } */ // if not we can directly have a look at the last par
      else if (item.equalsIgnoreCase("known problems") && (Main.getSession().getIsInfo()))
      {
         frame = new JInternalFrameKnownProblems();
      }
      else if (item.equalsIgnoreCase("stripe settings") && (Main.getSession().getIsAdmin()))
      {
         frame = new JInternalFrameStriping();
      }
      else if (item.equalsIgnoreCase("file browser") && (Main.getSession().getIsAdmin()))
      {
         frame = new JInternalFrameFileBrowser();
      }
      else if (item.equalsIgnoreCase("configuration") && (Main.getSession().getIsAdmin()))
      {
         frame = new JInternalFrameInstallationConfig();
      }
      else if (item.equalsIgnoreCase("install") && (Main.getSession().getIsAdmin()))
      {
         frame = new JInternalFrameInstall();
      }
      else if (item.equalsIgnoreCase("uninstall") && (Main.getSession().getIsAdmin()))
      {
         frame = new JInternalFrameUninstall();
      }
      else if (item.equalsIgnoreCase("start/stop daemon") &&
         (Main.getSession().getIsAdmin()))
      {
         frame = new JInternalFrameStartStop();
      }
      else if (item.equalsIgnoreCase("view log file") && (Main.getSession().getIsAdmin()))
      {
         frame = new JInternalFrameRemoteLogFiles(parserNodeList);
      }
      else if (item.equalsIgnoreCase("installation log file") && (Main.getSession().getIsAdmin()))
      {
         frame = new JInternalFrameLogFile();
      }
      else
      {
         return false;
      }

      if (!FrameManager.isFrameOpen((JInternalFrameInterface) frame)) {
         Main.getMainWindow().getDesktopPane().add(frame);
         frame.setVisible(true);
         FrameManager.addFrame((JInternalFrameInterface) frame);
      }

      return true;
   }

   final public void rebuildTree()
   {
      // delete everything
      TreePath topPath = this.getNextMatch("Menu", 0, null);
      DefaultMutableTreeNode topNode = (DefaultMutableTreeNode) topPath.getLastPathComponent();
      DefaultTreeModel model = (DefaultTreeModel) this.getModel();

      for (int i = topNode.getChildCount() - 1; i >= 0; i--)
      {
         DefaultMutableTreeNode n = (DefaultMutableTreeNode) topNode.getChildAt(i);
         model.removeNodeFromParent(n);
      }
 
      DefaultMutableTreeNode menuNode = new DefaultMutableTreeNode("Metadata nodes");
      model.insertNodeInto(menuNode, topNode, topNode.getChildCount() );
      model.insertNodeInto(new DefaultMutableTreeNode("Overview"), menuNode, menuNode.
         getChildCount());
      model.insertNodeInto(new DefaultMutableTreeNode("Node details"), menuNode, menuNode.
         getChildCount());

      menuNode = new DefaultMutableTreeNode("Storage nodes");
      model.insertNodeInto(menuNode, topNode, topNode.getChildCount() );
      model.insertNodeInto(new DefaultMutableTreeNode("Overview"), menuNode, menuNode.
         getChildCount());
      model.insertNodeInto(new DefaultMutableTreeNode("Node details"), menuNode, menuNode.
         getChildCount());

      menuNode = new DefaultMutableTreeNode("Client statistics");
      model.insertNodeInto(menuNode, topNode, topNode.getChildCount() );
      model.insertNodeInto(new DefaultMutableTreeNode("Metadata"), menuNode, menuNode.
         getChildCount());
      model.
         insertNodeInto(new DefaultMutableTreeNode("Storage"), menuNode, menuNode.getChildCount());

      menuNode = new DefaultMutableTreeNode("User statistics");
      model.insertNodeInto(menuNode, topNode, topNode.getChildCount() );
      model.insertNodeInto(new DefaultMutableTreeNode("Metadata"), menuNode, menuNode.
         getChildCount());
      model.
         insertNodeInto(new DefaultMutableTreeNode("Storage"), menuNode, menuNode.getChildCount());

      /*
      menuNode = new DefaultMutableTreeNode("Quota");
      model.insertNodeInto(menuNode, topNode, topNode.getChildCount() );
      model.insertNodeInto(new DefaultMutableTreeNode("Show Quota"), menuNode, menuNode.
         getChildCount());
      model.insertNodeInto(new DefaultMutableTreeNode("Set Quota Limits"), menuNode, menuNode
         .getChildCount());
      */

      menuNode = new DefaultMutableTreeNode("Management");
      model.insertNodeInto(menuNode, topNode, topNode.getChildCount() );
      model.insertNodeInto(new DefaultMutableTreeNode("Known Problems"), menuNode, menuNode.
         getChildCount());

      if (Main.getSession().getIsAdmin())
      {
         model.insertNodeInto(new DefaultMutableTreeNode("Start/Stop Daemon"), menuNode,
            menuNode.getChildCount());
         model.insertNodeInto(new DefaultMutableTreeNode("View Log File"), menuNode,
            menuNode.getChildCount());


         menuNode = new DefaultMutableTreeNode("FS Operations");
         model.insertNodeInto(menuNode, topNode, topNode.getChildCount() );
         model.insertNodeInto(new DefaultMutableTreeNode("Stripe Settings"), menuNode, menuNode.
            getChildCount());
         model.insertNodeInto(new DefaultMutableTreeNode("File Browser"), menuNode, menuNode.
            getChildCount());

         menuNode = new DefaultMutableTreeNode("Installation");
         model.insertNodeInto(menuNode, topNode, topNode.getChildCount() );
         model.insertNodeInto(new DefaultMutableTreeNode("Configuration"), menuNode, menuNode.
            getChildCount());
         model.insertNodeInto(new DefaultMutableTreeNode("Install"), menuNode, menuNode.
            getChildCount());
         model.insertNodeInto(new DefaultMutableTreeNode("Uninstall"), menuNode, menuNode.
            getChildCount());
         model.insertNodeInto(new DefaultMutableTreeNode("Installation Log File"), menuNode,
            menuNode.getChildCount());
      }

      this.expandPath(topPath);
   }

   public void startUpdateMenuThread()
   {
      updateThread.start();
   }

   public void stopUpdateMenuThread()
   {
      updateThread.shouldStop();
   }

   private class TreeMenuMouseListener extends MouseAdapter
   {
      @Override
      public void mousePressed(MouseEvent e)
      {
         JTreeMenu jTreeMenu = (JTreeMenu) e.getComponent();
         int selRow = jTreeMenu.getRowForLocation(e.getX(), e.getY());
         TreePath selPath = jTreeMenu.getPathForLocation(e.getX(), e.getY());
         if (selPath != null)
         {
            DefaultMutableTreeNode node = (DefaultMutableTreeNode)selPath.getLastPathComponent();
            if ((selRow != -1) && (node.isLeaf()))
            {
               if (e.getClickCount() >= 2)
               {
                  if (!openFrame(selPath))
                  {
                     logger.log(Level.WARNING, "Tried to open frame " + selPath.toString() +
                        " which does not exist", false);
                  }
               }
            }
         }
      }
   }

   private class MenuUpdateThread extends GuiThread
   {
      MenuUpdateThread(String name)
      {
         super(name);
      }

      @Override
      public void run()
      {
         long delay = 4000; //milliseconds

         while(true)
         {
            updateMetadataNodes();
            updateStorageNodes();

            try
            {
               sleep(delay);
            }
            catch (InterruptedException ex)
            {
               logger.log(Level.FINEST, "Internal error.", ex);
            }
         }
      }
   }
}
