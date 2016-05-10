#ifndef MESSAGINGTKARGS_H_
#define MESSAGINGTKARGS_H_


#define REQUESTRESPONSEARGS_LOGFLAG_PEERTRYAGAIN                 1 // peer asked for retry
#define REQUESTRESPONSEARGS_LOGFLAG_PEERINDIRECTCOMMM            2 /* peer encountered indirect comm
                                                                      error, suggests retry */
#define REQUESTRESPONSEARGS_LOGFLAG_PEERINDIRECTCOMMM_NOTAGAIN   4 /* peer encountered indirect comm
                                                                      error, suggests no retry */
#define REQUESTRESPONSEARGS_LOGFLAG_CONNESTABLISHFAILED          8 /* unable to establish conn. */
#define REQUESTRESPONSEARGS_LOGFLAG_RETRY                       16 /* communication retry */


/**
 * This class contains arguments for requestResponse() with a certain target.
 */
struct RequestResponseTarget
{
   /**
    * Constructor without targetStates and without mirror handling.
    */
   RequestResponseTarget(uint16_t targetID, TargetMapper* targetMapper,
      NodeStoreServers* nodeStore) :
         targetID(targetID), targetMapper(targetMapper), nodeStore(nodeStore),
         targetStates(NULL), mirrorBuddies(NULL), useBuddyMirrorSecond(false)
   {
      // see initializer list
   }

   /**
    * Constructor with targetStates and with mirror handling.
    */
   RequestResponseTarget(uint16_t targetID, TargetMapper* targetMapper,
      NodeStoreServers* nodeStore, TargetStateStore* targetStates,
      MirrorBuddyGroupMapper* mirrorBuddies, bool useBuddyMirrorSecond) :
         targetID(targetID), targetMapper(targetMapper), nodeStore(nodeStore),
         targetStates(targetStates), mirrorBuddies(mirrorBuddies),
         useBuddyMirrorSecond(useBuddyMirrorSecond)
   {
      // see initializer list
   }

   void setTargetStates(TargetStateStore* targetStates)
   {
      this->targetStates = targetStates;
   }

   void setMirrorInfo(MirrorBuddyGroupMapper* mirrorBuddies, bool useBuddyMirrorSecond)
   {
      this->mirrorBuddies = mirrorBuddies;
      this->useBuddyMirrorSecond = useBuddyMirrorSecond;
   }


   uint16_t targetID; // receiver
   TargetMapper* targetMapper; // for targetID -> nodeID lookup
   NodeStoreServers* nodeStore; // for nodeID lookup

   TargetStateStore* targetStates; /* if !NULL, check for state "good" or fail immediately if
                                      offline (other states handling depend on mirrorBuddies) */
   TargetReachabilityState outTargetReachabilityState; /* set to state of target by
                                      requestResponseX() if targetStates were given and
                                      FhgfsOpsErr_COMMUNICATION is returned */
   TargetConsistencyState outTargetConsistencyState;

   MirrorBuddyGroupMapper* mirrorBuddies; // if !NULL, the given targetID is a mirror group ID
   bool useBuddyMirrorSecond; // true to use secondary instead of primary
};

/**
 * This class contains arguments for requestResponse() with a certain node.
 */
struct RequestResponseNode
{
   /**
    * Constructor without targetStates and without mirror handling.
    */
   RequestResponseNode(uint16_t nodeID, NodeStoreServers* nodeStore) :
         nodeID(nodeID), nodeStore(nodeStore), targetStates(NULL), mirrorBuddies(NULL),
         useBuddyMirrorSecond(false)
   {
      // see initializer list
   }

   /**
    * Constructor with targetStates and with mirror handling.
    */
   RequestResponseNode(uint16_t nodeID, NodeStoreServers* nodeStore,
      TargetStateStore* targetStates, MirrorBuddyGroupMapper* mirrorBuddies,
      bool useBuddyMirrorSecond) :
         nodeID(nodeID), nodeStore(nodeStore), targetStates(targetStates),
         mirrorBuddies(mirrorBuddies), useBuddyMirrorSecond(useBuddyMirrorSecond)
   {
      // see initializer list
   }

   void setTargetStates(TargetStateStore* targetStates)
   {
      this->targetStates = targetStates;
   }

   void setMirrorInfo(MirrorBuddyGroupMapper* mirrorBuddies, bool useBuddyMirrorSecond)
   {
      this->mirrorBuddies = mirrorBuddies;
      this->useBuddyMirrorSecond = useBuddyMirrorSecond;
   }

   uint16_t nodeID; // receiver
   NodeStoreServers* nodeStore; // for nodeID lookup

   TargetStateStore* targetStates; /* if !NULL, check for state "good" or fail immediately if
                                      offline (other states handling depend on mirrorBuddies) */
   TargetReachabilityState outTargetReachabilityState; /* set to state of node by requestResponseX()
                                      if targetStates were given and FhgfsOpsErr_COMMUNICATION is
                                      returned */
   TargetConsistencyState outTargetConsistencyState;

   MirrorBuddyGroupMapper* mirrorBuddies; // if !NULL, the given targetID is a mirror group ID
   bool useBuddyMirrorSecond; // true to use secondary instead of primary
};

/**
 * Arguments for request-response communication.
 *
 * Note: Destructor will free response msg and response buf, if they are not NULL.
 */
struct RequestResponseArgs
{
   /**
    * @param node may be NULL, depending on which requestResponse...() method is used.
    * @param respMsgType expected type of response message (NETMSGTYPE_...)
    */
   RequestResponseArgs(Node* node, NetMessage* requestMsg, unsigned respMsgType,
         FhgfsOpsErr (*sendExtraData)(Socket*, void*) = NULL, void* extraDataContext = NULL)
      : node(node), requestMsg(requestMsg), respMsgType(respMsgType), outRespBuf(NULL),
        outRespMsg(NULL), logFlags(0), sendExtraData(sendExtraData),
        extraDataContext(extraDataContext)
   {
      // see initializer list
   }

   ~RequestResponseArgs()
   {
      SAFE_DELETE_NOSET(outRespMsg);
      SAFE_FREE_NOSET(outRespBuf);
   }

   Node* node; // receiver

   NetMessage* requestMsg;
   unsigned respMsgType; // expected type of response message (NETMSGTYPE_...)

   char* outRespBuf; // the buffer from which the outRespMsg was deserialized (only set on success)
   NetMessage* outRespMsg; // received response message (only set on success)

   // internal (initialized by MessagingTk_requestResponseWithRRArgs() )
   unsigned char logFlags; // REQUESTRESPONSEARGS_LOGFLAG_... combination to avoid double-logging

   // hook to send extra data after the message
   FhgfsOpsErr (*sendExtraData)(Socket*, void*);
   void* extraDataContext;
};


#endif /* MESSAGINGTKARGS_H_ */
