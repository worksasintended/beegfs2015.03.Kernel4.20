package com.beegfs.admon.gui.common.nodes;

import com.beegfs.admon.gui.common.enums.NodeTypesEnum;
import java.util.ArrayList;
import java.util.Iterator;


public class Nodes implements Iterable<Node>
{
   private final static int INITIAL_CAPACITY = 10;
   private final ArrayList<Node> nodes;

   public Nodes()
   {
      nodes = new ArrayList<>(INITIAL_CAPACITY);
   }

   public boolean contains(Node node)
   {
      for (Node tmpNode : nodes)
      {
         if (node.equals(tmpNode))
         {
           return true;
         }
      }
      return false;
   }

   public boolean contains(String nodeID, NodeTypesEnum type)
   {
      for (Node tmpNode : nodes)
      {
         if (type == tmpNode.getType() && nodeID.equals(tmpNode.getNodeID()))
         {
           return true;
         }
      }
      return false;
   }

   public boolean contains(int nodeNumID, NodeTypesEnum type)
   {
      for (Node tmpNode : nodes)
      {
         if (type == tmpNode.getType() && nodeNumID == tmpNode.getNodeNumID())
         {
           return true;
         }
      }
      return false;
   }

   public Node getNode(String nodeID, NodeTypesEnum type)
   {
      for (Node node : nodes)
      {
         if (type == node.getType() && node.getNodeID().equals(nodeID))
         {
           return node;
         }
      }
      return null;
   }

   public Node getNode(int nodeNumID, NodeTypesEnum type)
   {
      for (Node node : nodes)
      {
         if (type == node.getType() && node.getNodeNumID() == nodeNumID)
         {
            return node;
         }
      }
      return null;
   }

   public Nodes getNodes(String group, NodeTypesEnum type)
   {
      Nodes groupNodes = new Nodes();

      for (Node node : nodes)
      {
         if (type == node.getType() && node.getGroup().equals(group))
         {
            groupNodes.add(node);
         }
      }
      return groupNodes;
   }

   public boolean add(Node node)
   {
      if (!nodes.contains(node))
      {
         return nodes.add(node);
      }
      else
      {
         return true;
      }
   }

   public boolean addOrUpdateNode(Node node)
   {
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

   public boolean addNodes(Nodes nodeList)
   {
      boolean retVal = true;
      boolean oneFound = false;
      
      for(Node node : nodeList)
      {
         if (this.add(node))
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

   public boolean remove(Node node)
   {
      return nodes.remove(node);
   }

   public boolean remove(String nodeID, NodeTypesEnum type)
   {
      for (Node node : nodes)
      {
         if (type  == node.getType() && node.getNodeID().equals(nodeID))
         {
            return nodes.remove(node);
         }
      }
      return false;
   }

   public boolean remove(int nodeNumID, NodeTypesEnum type)
   {
      for (Node node : nodes)
      {
         if (type  == node.getType() && node.getNodeNumID() == nodeNumID)
         {
            return nodes.remove(node);
         }
      }
      return false;
   }

   public boolean removeNodes(String group)
   {
      boolean retVal = true;
      boolean oneFound = false;

      for (Node node : nodes)
      {
         if (node.getGroup().equals(group))
         {
            if (this.remove(node))
            {
               oneFound = true;
            }
            else
            {
               retVal = false;
               oneFound = true;
            }
         }
      }

      if (!oneFound)
      {
         retVal = false;
      }
      
      return retVal;
   }

   public boolean removeNodes(Nodes nodeList)
   {
      boolean retVal = true;
      boolean oneFound = false;

      for (Node node : nodeList)
      {
         if (this.remove(node))
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

   public void clear()
   {
      nodes.clear();
   }

   @Override
   public Iterator<Node> iterator()
   {
      return nodes.iterator();
   }
}
