#ifndef HBMGRNOTIFICATION_H_
#define HBMGRNOTIFICATION_H_

#include <common/Common.h>


/*
 * This file contains an abstract base class for async notifications and also the various tiny
 * sub-classes.
 */


enum HbMgrNotificationType
   {HbMgrNotificationType_NODEADDED=0, HbMgrNotificationType_NODEREMOVED=1,
    HbMgrNotificationType_TARGETADDED=2, HbMgrNotificationType_REFRESHCAPACITYPOOLS=3,
    HbMgrNotificationType_REFRESHTARGETSTATES=4, HbMgrNotificationType_BUDDYGROUPADDED=5};



/**
 * Base class for async notifications
 */
class HbMgrNotification
{
   public:
      virtual ~HbMgrNotification() {}

      virtual void processNotification() = 0;


   protected:
      HbMgrNotification(HbMgrNotificationType notificationType) :
         notificationType(notificationType) {}

      HbMgrNotificationType notificationType;


   public:
      // getters & setters

      HbMgrNotificationType getNotificationType()
      {
         return notificationType;
      }
};


class HbMgrNotificationNodeAdded : public HbMgrNotification
{
   public:
      HbMgrNotificationNodeAdded(std::string nodeID, uint16_t nodeNumID, NodeType nodeType) :
         HbMgrNotification(HbMgrNotificationType_NODEADDED),
         nodeID(nodeID), nodeNumID(nodeNumID), nodeType(nodeType)
      {}

      void processNotification();


   private:
      std::string nodeID;
      uint16_t nodeNumID;
      NodeType nodeType;

      void propagateAddedNode(Node* node);
};


class HbMgrNotificationNodeRemoved : public HbMgrNotification
{
   public:
      HbMgrNotificationNodeRemoved(std::string nodeID, uint16_t nodeNumID, NodeType nodeType) :
         HbMgrNotification(HbMgrNotificationType_NODEREMOVED),
         nodeID(nodeID), nodeNumID(nodeNumID), nodeType(nodeType)
      {}

      void processNotification();


   private:
      std::string nodeID;
      uint16_t nodeNumID;
      NodeType nodeType;

      void propagateRemovedNode();
};


class HbMgrNotificationTargetAdded : public HbMgrNotification
{
   public:
      HbMgrNotificationTargetAdded(uint16_t targetID, uint16_t nodeID) :
         HbMgrNotification(HbMgrNotificationType_TARGETADDED),
         targetID(targetID), nodeID(nodeID)
      {}

      void processNotification();


   private:
      uint16_t targetID;
      uint16_t nodeID;

      void propagateAddedTarget();
};

class HbMgrNotificationRefreshCapacityPools : public HbMgrNotification
{
   public:
      HbMgrNotificationRefreshCapacityPools() :
         HbMgrNotification(HbMgrNotificationType_REFRESHCAPACITYPOOLS)
      {}

      void processNotification();


   private:
      void propagateRefreshCapacityPools();
};

class HbMgrNotificationRefreshTargetStates : public HbMgrNotification
{
   public:
      HbMgrNotificationRefreshTargetStates() :
         HbMgrNotification(HbMgrNotificationType_REFRESHTARGETSTATES)
      {}

      void processNotification();


   private:
      void propagateRefreshTargetStates();
};

class HbMgrNotificationMirrorBuddyGroupAdded: public HbMgrNotification
{
   public:
      HbMgrNotificationMirrorBuddyGroupAdded(uint16_t buddyGroupID, uint16_t primaryTargetID,
         uint16_t secondaryTargetID) :
         HbMgrNotification(HbMgrNotificationType_BUDDYGROUPADDED), buddyGroupID(buddyGroupID),
         primaryTargetID(primaryTargetID), secondaryTargetID(secondaryTargetID)
      {
      }

      void processNotification();

   private:
      uint16_t buddyGroupID;
      uint16_t primaryTargetID;
      uint16_t secondaryTargetID;

      void propagateAddedMirrorBuddyGroup();
};

#endif /* HBMGRNOTIFICATION_H_ */
