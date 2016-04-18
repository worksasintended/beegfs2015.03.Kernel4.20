#ifndef MAILER_H_
#define MAILER_H_

#include <common/Common.h>
#include <toolkit/Mail.h>
#include <components/ExternalJobRunner.h>

struct DownNode
{
   uint16_t nodeNumID;
   std::string nodeID;
   int64_t downSince;
   bool eMailSent;
};

typedef std::list<DownNode> DownNodeList;
typedef std::list<DownNode>::iterator DownNodeListIter;

struct DownNodesContainer
{
      DownNodeList downMetaNodes;
      DownNodeList downStorageNodes;
      int64_t lastMail;
      bool newDownNodes;
};

class Mailer : public PThread
{
   public:
      Mailer();
      bool addDownNode(uint16_t nodeNumID, std::string nodeID, NodeType nodeType);
      virtual ~Mailer() { }

   protected:
      LogContext log;

   private:
      DownNodesContainer downNodes;
      void run();
      void notifyDownNodes();
      void notifyBackUpNodes();

   public:
      // public inliners

      static std::string getHumanReadableNodeID(DownNode downNode)
      {
         return downNode.nodeID + " [ID: " + StringTk::uintToStr(downNode.nodeNumID) + "]";
      }
};

#endif /*MAILER_H_*/
