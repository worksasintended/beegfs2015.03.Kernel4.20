#ifndef REGISTERNODEMSGEX_H_
#define REGISTERNODEMSGEX_H_

#include <common/net/message/nodes/RegisterNodeMsg.h>
#include <common/nodes/NodeStoreServers.h>

class RegisterNodeMsgEx : public RegisterNodeMsg
{
   public:
      RegisterNodeMsgEx() : RegisterNodeMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

      static Node* constructNode(std::string nodeID, uint16_t nodeNumID,
         unsigned short portUDP, unsigned short portTCP, NicAddressList& nicList);
      static bool checkNewServerAllowed(NodeStoreServers* nodeStore, uint16_t nodeNumID,
         NodeType nodeType);
      static void processIncomingRoot(uint16_t rootNumID, NodeType nodeType);
      static void processNewNode(std::string nodeID, uint16_t nodeNumID, NodeType nodeType,
         unsigned fhgfsVersion, NicAddressList* nicList, std::string sourcePeer);


   protected:

   private:
};

#endif /* REGISTERNODEMSGEX_H_ */
