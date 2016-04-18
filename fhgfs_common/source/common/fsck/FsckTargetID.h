#ifndef FSCKTARGETID_H_
#define FSCKTARGETID_H_

#include <common/Common.h>

enum FsckTargetIDType
{
   FsckTargetIDType_TARGET = 0,
   FsckTargetIDType_BUDDYGROUP = 1
};

class FsckTargetID
{
   public:
      size_t serialize(char* outBuf);
      bool deserialize(const char* buf, size_t bufLen, unsigned* outLen);
      unsigned serialLen(void);

   private:
      uint16_t id;
      FsckTargetIDType targetIDType;

   public:
      FsckTargetID(uint16_t id, FsckTargetIDType targetIDType) : id(id), targetIDType(targetIDType)
      {
         // all initialization done in initialization list
      };

      //only for deserialization
      FsckTargetID() {}

      uint16_t getID() const
      {
         return id;
      }

      FsckTargetIDType getTargetIDType() const
      {
         return targetIDType;
      }

      bool operator< (const FsckTargetID& other)
      {
         if (id < other.id)
            return true;
         else
            return false;
      }

      bool operator== (const FsckTargetID& other)
      {
         if (id != other.id)
            return false;
         else
         if (targetIDType != other.targetIDType)
            return false;
         else
            return true;
      }

      bool operator!= (const FsckTargetID& other)
      {
         return !(operator==( other ) );
      }

      void update(uint16_t id, FsckTargetIDType targetIDType)
       {
          this->id = id;
          this->targetIDType = targetIDType;
       }
};

typedef std::list<FsckTargetID> FsckTargetIDList;
typedef FsckTargetIDList::iterator FsckTargetIDListIter;

#endif /* FSCKTARGETID_H_ */
