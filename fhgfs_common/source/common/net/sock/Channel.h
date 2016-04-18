#ifndef CHANNEL_H_
#define CHANNEL_H_

#include <common/toolkit/poll/Pollable.h>
#include <common/Common.h>


class Channel : public Pollable
{
   public:
      
      
   protected:
      Channel()
      {
         this->isDirect = true;
         this->hasActivity = true; // initially active to avoid immediate disconnection
         this->isAuthenticated = false;
      }
      
      virtual ~Channel() {}
      
      
   private:
      bool isDirect; // true if used for direct work requests only (wrt to worker types)
      bool hasActivity; // true if channel was not idle
      bool isAuthenticated; // true if valid authentication message received
      
      
   public:
      // getters & setters
      inline bool getIsDirect() const
      {
         return isDirect;
      }
      
      inline void setIsDirect(bool isDirect)
      {
         this->isDirect = isDirect;
      }

      inline bool getHasActivity()
      {
         return hasActivity;
      }

      inline void setHasActivity()
      {
         this->hasActivity = true;
      }

      inline void resetHasActivity()
      {
         this->hasActivity = false;
      }

      inline bool getIsAuthenticated() const
      {
         return isAuthenticated;
      }

      inline void setIsAuthenticated()
      {
         this->isAuthenticated = true;
      }
};


#endif /*CHANNEL_H_*/
