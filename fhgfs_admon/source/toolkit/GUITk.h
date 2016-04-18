/*
 * GuiTk provides static functions to help performing GUI-related operations. At the moment these
 * are only functions for doing the additional administrator authentication of the GUI with a
 * nonce-based challenge-response algorithm
 */

#ifndef GUITK_H_
#define GUITK_H_

#include <common/toolkit/StringTk.h>
#include <common/threading/SafeMutexLock.h>
#include "Crypt.h"

class GUITk
{
   public:
      GUITk() { }

      static int createNonce(int64_t *outNonce);
      static void clearNonce(int nonceID);
      static void clearNonceLocked(int nonceID);
      static bool checkReply(std::string reply, std::string secret, int nonceID);
      static std::string cryptWithNonce(std::string str, int nonceID);
      static void cryptWithNonce(StringList *inList, StringList *outList, int nonceID);

   private:
      static Mutex nonceMutex;
      static std::map<int, int64_t> nonces;
      static std::string str_AND(std::string str1, std::string str2);
};

#endif /* GUITK_H_ */
