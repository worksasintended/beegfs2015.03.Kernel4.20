#ifndef MESSAGINGTK_H_
#define MESSAGINGTK_H_

#include <common/app/AbstractApp.h>
#include <common/app/log/LogContext.h>
#include <common/net/message/NetMessage.h>
#include <common/net/sock/Socket.h>
#include <common/net/message/AbstractNetMessageFactory.h>
#include <common/nodes/NodeStoreServers.h>
#include <common/nodes/MirrorBuddyGroupMapper.h>
#include <common/nodes/Node.h>
#include <common/threading/PThread.h>
#include <common/toolkit/MessagingTkArgs.h>
#include <common/Common.h>


#define MESSAGINGTK_NUM_COMM_RETRIES   1 /* 1 retry in case the connection was already broken when
                                            we got it (e.g. peer daemon restart) */
#define MESSAGINGTK_INFINITE_RETRY_WAIT_MS (5000) // how long to wait if peer asks for retry


class MessagingTk
{
   public:
      static bool requestResponse(RequestResponseArgs* rrArgs);
      static bool requestResponse(Node* node, NetMessage* requestMsg, unsigned respMsgType,
         char** outRespBuf, NetMessage** outRespMsg);
      static FhgfsOpsErr requestResponseNode(RequestResponseNode* rrNode,
         RequestResponseArgs* rrArgs);
      static FhgfsOpsErr requestResponseTarget(RequestResponseTarget* rrTarget,
         RequestResponseArgs* rrArgs);

      static unsigned recvMsgBuf(Socket* sock, char** outBuf);
      static char* createMsgBuf(NetMessage* msg);
   

   private:
      MessagingTk() {}
      
      static FhgfsOpsErr requestResponseComm(RequestResponseArgs* rrArgs);
      static FhgfsOpsErr handleGenericResponse(RequestResponseArgs* rrArgs);

   public:
      // inliners
      


      /**
       * Receive a message into a pre-alloced buffer.
       * 
       * @return 0 on error (e.g. message to big), message length otherwise
       * @throw SocketException on communication error
       */ 
      static unsigned recvMsgBuf(Socket* sock, char* bufIn, size_t bufInLen)
      {
         const char* logContext = "MessagingTk (recv msg in-buf";

         const int recvTimeoutMS = CONN_LONG_TIMEOUT;
         
         unsigned numReceived = 0;
      
         // receive at least the message header
         
         numReceived += sock->recvExactT(
            bufIn, NETMSG_MIN_LENGTH, 0, recvTimeoutMS);
         
         unsigned msgLength = NetMessage::extractMsgLengthFromBuf(bufIn);
         
         if(unlikely(msgLength > bufInLen) )
         { // message too big to be accepted
            LogContext(logContext).log(Log_NOTICE,
               std::string("Received a message that is too large from: ") +
               sock->getPeername() );
            
            return 0;
         }
         
         // receive the rest of the message
      
         if(msgLength > numReceived)
            sock->recvExactT(&bufIn[numReceived], msgLength-numReceived, 0, recvTimeoutMS);
         
         return msgLength;
      }
      
};

#endif /*MESSAGINGTK_H_*/
