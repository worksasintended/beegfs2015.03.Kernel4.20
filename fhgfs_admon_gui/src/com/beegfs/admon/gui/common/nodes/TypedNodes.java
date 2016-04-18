package com.beegfs.admon.gui.common.nodes;

import com.beegfs.admon.gui.common.enums.NodeTypesEnum;
import java.util.ArrayList;

public class TypedNodes extends Nodes
{
   private final static int INITIAL_CAPACITY = 10;

   private final ArrayList<Node> nodes;
   private final NodeTypesEnum nodeType;

   public TypedNodes(NodeTypesEnum nodeType)
   {
      nodes = new ArrayList<>(INITIAL_CAPACITY);
      this.nodeType = nodeType;
   }

   public boolean contains(String nodeID)
   {
      for (Node tmpNode : nodes)
      {
         if (nodeID.equals(tmpNode.getNodeID()))
         {
           return true;
         }
      }
      return false;
   }

   public boolean contains(int nodeNumID)
   {
      for (Node tmpNode : nodes)
      {
         if (nodeNumID == tmpNode.getNodeNumID())
         {
           return true;
         }
      }
      return false;
   }

   public Node getNode(String nodeID)
   {
      for (Node node : nodes)
      {
         if (node.getNodeID().equals(nodeID))
         {
           return node;
         }
      }
      return null;
   }

   public Node getNode(int nodeNumID)
   {
      for (Node node : nodes)
      {
         if (node.getNodeNumID() == nodeNumID)
         {
            return node;
         }
      }
      return null;
   }

   public Nodes getNodes(String group)
   {
      Nodes groupNodes = new Nodes();

      for (Node node : nodes)
      {
         if (node.getGroup().equals(group))
         {
            groupNodes.add(node);
         }
      }
      return groupNodes;
   }

   @Override
   public boolean add(Node node)
   {
      if (this.nodeType != node.getType())
      {
         return false;
      }
      
      if (!nodes.contains(node))
      {
         return nodes.add(node);
      }
      else
      {
         return true;
      }
   }

   @Override
   public boolean addOrUpdateNode(Node node)
   {
      if (this.nodeType != node.getType())
      {
         return false;
      }
      
      if (nodes.contains(node))
      {
         Node tmpNode = this.getNode(node.getNodeNumID(), node.getType());
         tmpNode.setNodeID(node.getNodeID());
         tmpNode.setNodeNumID(node.getNodeNumID());
         tmpNode.setGroup(node.getGroup());
         tmpNode.setType(node.getType());
         return true;
      }
      else
      {
         return this.add(node);
      }
   }

   @Override
   public boolean addNodes(Nodes nodeList)
   {
      boolean retVal = true;
      boolean oneFound = false;

      for(Node node : nodeList)
      {
         if (nodes.add(node))
         {
            oneFound = true;
         }
         else
         {
            retVal = false;
            oneFound = true;
         }
      }

      if (!oneFound)
      {
         retVal = false;
      }

      return retVal;
   }

   @Override
   public boolean addOrUpdateNodes(Nodes nodeList)
   {
      boolean retVal = true;
      boolean oneFound = false;

      for(Node node : nodeList)
      {
         if (this.addOrUpdateNode(node))
         {
            oneFound = true;
         }
         else
         {
            retVal = false;
            oneFound = true;
         }
      }

      if (!oneFound)
      {
         retVal = false;
      }

      return retVal;
   }
   
   public boolean remove(String nodeID)
   {
      for (Node node : nodes)
      {
         if (node.getNodeID().equals(nodeID))
         {
            return nodes.remove(node);
         }
      }
      return false;
   }

   public boolean remove(int nodeNumID)
   {
      for (Node node : nodes)
      {
         if (node.getNodeNumID() == nodeNumID)
         {
            return nodes.remove(node);
         }
      }
      return false;
   }

   @Override
   public boolean removeNodes(Nodes nodeList)
   {
      boolean retVal = true;
      boolean oneFound = false;

      for (Node node : nodeList)
      {
         if (nodes.remove(node))
         {
            oneFound = true;
         }
         else
         {
            retVal = false;
            oneFound = true;
         }
      }

      if (!oneFound)
      {
         retVal = false;
      }

      return retVal;
   }
}
