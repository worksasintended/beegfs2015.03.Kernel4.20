#include <program/Program.h>
#include "Mailer.h"

Mailer::Mailer() : PThread("Mailer")
{
   log.setContext("Mailer");
}

bool Mailer::addDownNode(uint16_t nodeNumID, std::string nodeID, NodeType nodeType)
{
   std::string nodeTypeStr = "";
   DownNodeList *list = NULL;

   if(nodeType == NODETYPE_Meta)
   {
      list = &(this->downNodes.downMetaNodes);
      nodeTypeStr = "meta";
   }
   else
   if(nodeType == NODETYPE_Storage)
   {
      list = &(this->downNodes.downStorageNodes);
      nodeTypeStr = "storage";
   }
   else
   {
      return false;
   }

   for(DownNodeListIter iter = list->begin(); iter != list->end(); iter++)
   {
      if ( (*iter).nodeNumID == nodeNumID )
         return false;
   }

   DownNode downNode;
   downNode.nodeNumID = nodeNumID;
   downNode.nodeID = nodeID;
   downNode.downSince = time(NULL);
   downNode.eMailSent = false;
   list->push_back(downNode);

   // fire up a script if it is set in runtimeConfig
   RuntimeConfig *runtimeCfg = Program::getApp()->getRuntimeConfig();
   std::string scriptPath = runtimeCfg->getScriptPath();

   if(!scriptPath.empty())
   {
      ExternalJobRunner *jobRunner = Program::getApp()->getJobRunner();

      Mutex mutex;
      SafeMutexLock mutexLock(&mutex);

      Job *job = jobRunner->addJob(scriptPath + " " + nodeTypeStr + " " + nodeID, &mutex);

      while (!job->finished)
         job->jobFinishedCond.wait(&mutex);

      mutexLock.unlock();

      jobRunner->delJob(job->id);
   }

   return true;
}

void Mailer::run()
{
   unsigned intervalMS = Program::getApp()->getConfig()->getMailCheckIntervalTimeSec() * 1000;

   try
   {
      log.log(Log_DEBUG, "Component started.");
      registerSignalHandler();
      while (!getSelfTerminate())
      {
         RuntimeConfig *runtimeCfg = Program::getApp()->getRuntimeConfig();
         if(runtimeCfg->getMailEnabled())
         {
            notifyBackUpNodes();
            notifyDownNodes();
         }

         // do nothing but wait for the time of intervalMS
         if(PThread::waitForSelfTerminateOrder(intervalMS))
         {
            break;
         }
      }
      log.log(Log_DEBUG, "Component stopped.");

   }
   catch (std::exception& e)
   {
      Program::getApp()->handleComponentException(e);
   }
}

void Mailer::notifyDownNodes()
{
   RuntimeConfig *runtimeCfg = Program::getApp()->getRuntimeConfig();
   int minDownTimeSec = runtimeCfg->getMailMinDownTimeSec();
   int resendMailTimeMin = runtimeCfg->getMailResendMailTimeMin();
   bool notifiedNewDownNodes = false;
   bool sendMail = false;

   std::string msg = "Some Nodes in the BeeGFS System monitored by " +
      Program::getApp()->getLocalNode()->getTypedNodeID() +" seem to be dead!\r\n\r\n";

   msg += "A list of these hosts follows : \r\n\r\n Meta Nodes :"
      "\r\n----------------\r\n\r\n";

   int64_t nowTime = time (NULL);
   for(DownNodeListIter iter = downNodes.downMetaNodes.begin();
      iter != downNodes.downMetaNodes.end(); iter++)
   {
      if ( (nowTime - (*iter).downSince) > (minDownTimeSec) )
      {
         msg += Mailer::getHumanReadableNodeID((*iter)) +" / Down since " +
            StringTk::timespanToStr(nowTime - (*iter).downSince)+".\r\n";
         sendMail = true;

         if (!(*iter).eMailSent)
         {
            notifiedNewDownNodes = true;
            (*iter).eMailSent = true;
         }
      }
   }

   msg += "\r\n\r\n\r\n Storage Nodes :\r\n----------------\r\n\r\n";

   for(DownNodeListIter iter=downNodes.downStorageNodes.begin();
      iter!=downNodes.downStorageNodes.end(); iter++)
   {
      if ( (nowTime - (*iter).downSince) > (minDownTimeSec) )
      {
         msg += Mailer::getHumanReadableNodeID((*iter)) +" / Down since " +
            StringTk::timespanToStr(nowTime - (*iter).downSince)+".\r\n";
         sendMail = true;

         if (!(*iter).eMailSent)
         {
            notifiedNewDownNodes = true;
            (*iter).eMailSent = true;
         }
      }
   }

   if (notifiedNewDownNodes ||
      (sendMail && ((nowTime - downNodes.lastMail) > (resendMailTimeMin * 60))) )
   {
      log.log(Log_SPAM, "Sending eMail caused by down nodes");
      std::string subject = "BeeGFS: Nodes Down";
      Mail::sendMail(runtimeCfg->getMailSmtpServer(),
         runtimeCfg->getMailSender(), runtimeCfg->getMailRecipient(),
         subject, msg);

      downNodes.lastMail = nowTime;
   }
}

void Mailer::notifyBackUpNodes()
{
   RuntimeConfig *runtimeCfg = Program::getApp()->getRuntimeConfig();
   bool sendMail = false;

   std::string msg = "Some nodes in the BeeGFS System monitored by " +
      Program::getApp()->getLocalNode()->getID() + " seem to be up again after "
      "a downtime!\r\n\r\n";
   msg += "A list of these hosts follows : \r\n\r\n Meta Nodes :"
      "\r\n----------------\r\n\r\n";

   int64_t nowTime = time (NULL);

   //actively poll for nodes which are reachable again
   NodeStoreMetaEx *metaNodeStore = Program::getApp()->getMetaNodes();
   DownNodeListIter iter=downNodes.downMetaNodes.begin();

   while (iter!=downNodes.downMetaNodes.end())
   {
      MetaNodeEx *node = (MetaNodeEx*)metaNodeStore->referenceNode((*iter).nodeNumID);
      if (node==NULL) // node seems to be removed from store
      {
            iter = downNodes.downMetaNodes.erase(iter);
      }
      else
      {
         if ( node->getContent().isResponding )
         {
            msg += Mailer::getHumanReadableNodeID((*iter)) +
               " / Was down for " + StringTk::timespanToStr(
               nowTime - (*iter).downSince)+".\r\n";
            iter = downNodes.downMetaNodes.erase(iter);
            sendMail = true;
            node->upAgain();
         }
         else
         {
            iter++;
         }

         metaNodeStore->releaseMetaNode(&node);
      }
   }

   msg += "\r\n\r\n\r\n Storage Nodes :\r\n----------------\r\n\r\n";

    NodeStoreStorageEx *storageNodeStore = Program::getApp()->getStorageNodes();
   iter = downNodes.downStorageNodes.begin();

    while (iter != downNodes.downStorageNodes.end() )
   {
      StorageNodeEx *node = (StorageNodeEx *)storageNodeStore->referenceNode((*iter).nodeNumID);

      if ( node->getContent().isResponding )
      {
         msg += Mailer::getHumanReadableNodeID((*iter)) +" / Was down for " +
            StringTk::timespanToStr(nowTime - (*iter).downSince)+".\r\n";
         iter = downNodes.downStorageNodes.erase(iter);
         sendMail = true;
         node->upAgain();
      }
      else
      {
         iter++;
      }

      storageNodeStore->releaseStorageNode(&node);
   }

    if (sendMail)
    {
       log.log(5,"Sending eMail caused by nodes which are up again");
       std::string subject = "BeeGFS : Nodes Up again";
       Mail::sendMail(runtimeCfg->getMailSmtpServer(),
          runtimeCfg->getMailSender(), runtimeCfg->getMailRecipient(),
          subject, msg);
    }
}
