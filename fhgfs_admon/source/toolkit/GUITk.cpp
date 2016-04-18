#include "GUITk.h"

std::map<int,int64_t> GUITk::nonces;
Mutex GUITk::nonceMutex;

std::string GUITk::str_AND(std::string str1, std::string str2)
{
    std::string retval(str1);

    short unsigned int a_len=str1.length();
    short unsigned int b_len=str2.length();
    short unsigned int i=0;
    short unsigned int j=0;

    for(i=0;i<a_len;i++)
    {
        retval[i]=str1[i] & str2[i];
        j=(++j<b_len?j:0);
    }

    return retval;
}

int GUITk::createNonce(int64_t *outNonce)
{
   /* initialize random seed: */
	  srand ( time(NULL) );
	  int64_t intNonce = rand();
      int id = 0;

      SafeMutexLock nonceMutexLock(&nonceMutex);

      std::map<int,int64_t>::iterator iter = GUITk::nonces.find(id);
      while (iter != GUITk::nonces.end())
      {
    	 id = rand() % 10000;
    	 iter = GUITk::nonces.find(id);
      }
      GUITk::nonces[id] = intNonce;
      *outNonce = intNonce;

      nonceMutexLock.unlock();

      return id;
}

void GUITk::clearNonceLocked(int nonceID)
{
   SafeMutexLock nonceMutexLock(&nonceMutex);
	GUITk::nonces.erase(nonceID);
	nonceMutexLock.unlock();
}

void GUITk::clearNonce(int nonceID)
{
   GUITk::nonces.erase(nonceID);
}

bool GUITk::checkReply(std::string reply, std::string secret, int nonceID)
{
   SafeMutexLock nonceMutexLock(&nonceMutex);

   std::string expected = Crypt::DoMD5(secret+StringTk::int64ToStr(GUITk::nonces[nonceID]));
   GUITk::clearNonce(nonceID);

   nonceMutexLock.unlock();

   return (reply.compare(expected) == 0);
}

std::string GUITk::cryptWithNonce(std::string str, int nonceID)
{
   SafeMutexLock nonceMutexLock(&nonceMutex);

   std::string crypted = Crypt::DoMD5(str+StringTk::int64ToStr(GUITk::nonces[nonceID]));
   GUITk::clearNonce(nonceID);

   nonceMutexLock.unlock();
   return crypted;
}

void GUITk::cryptWithNonce(StringList *inList, StringList *outList, int nonceID)
{
   SafeMutexLock nonceMutexLock(&nonceMutex);

	for (StringListIter iter = inList->begin(); iter != inList->end(); iter++)
	{
        std::string str = *iter;
        std::string crypted = Crypt::DoMD5(str+StringTk::int64ToStr(GUITk::nonces[nonceID]));
        outList->push_back(crypted);
	}
   GUITk::clearNonce(nonceID);

   nonceMutexLock.unlock();
}
