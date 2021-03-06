#ifndef LOGCONTEXT_H_
#define LOGCONTEXT_H_

#include "Logger.h"
//#include "AbstractApp.h"
#include "Common.h"

#include <execinfo.h>

// we don't include Main.h, as it has static defines and the compiler issues warnings about
// undefined functions
extern Logger* getGlobalLogger();

#define LOGCONTEXT_BACKTRACE_ARRAY_SIZE   32

#ifdef LOG_DEBUG_MESSAGES

   #define LOG_DEBUG(contextStr, level, msgStr) \
      do { LogContext(contextStr).log(level, msgStr); } while(0)

   #define LOG_DEBUG_CONTEXT(logContext, level, msgStr) \
      do { (logContext).log(level, msgStr); } while(0)

   #define LOG_DEBUG_BACKTRACE() \
      do { LogContext(__func__).logBacktrace(); } while(0)

#else

   #define LOG_DEBUG(contextStr, level, msgStr) \
      do { /* nothing */ } while(0)

   #define LOG_DEBUG_CONTEXT(logContext, level, msgStr) \
      do { /* nothing */ } while(0)

   #define LOG_DEBUG_BACKTRACE() \
      do { /* nothing */ } while(0)

#endif // LOG_DEBUG_MESSAGES


class LogContext
{
   public:
      LogContext(const std::string contextStr="<undefined>") : contextStr(contextStr)
      {
         this->logger = getGlobalLogger();
      }
      
      /**
       * @param noEOL do not print an end of line and also not the logContext
       */
      void log(int level, const char* msg, bool noEOL = false)
      {
         if(unlikely(!logger) )
            return;
            
         logger->log(level, contextStr.c_str(), msg, noEOL);
      }

      /**
       * @param noEOL do not print an end of line and also not the logContext
       */
      void log(int level, const std::string msg, bool noEOL = false)
      {
         log(level, msg.c_str(), noEOL);
      }
      
      void logErr(const char* msg)
      {
         if(unlikely(!logger) )
            return;
            
         logger->logErr(contextStr.c_str(), msg);
      }

      void logErr(const std::string msg)
      {
         logErr(msg.c_str() );
      }
      
      void logBacktrace()
      {
         int backtraceLength = 0;
         char** backtraceSymbols = NULL;

         void* backtraceArray[LOGCONTEXT_BACKTRACE_ARRAY_SIZE];
         backtraceLength = backtrace(backtraceArray, LOGCONTEXT_BACKTRACE_ARRAY_SIZE);

         // note: symbols are malloc'ed and need to be freed later
         backtraceSymbols = backtrace_symbols(backtraceArray, backtraceLength);

         logger->logBacktrace(contextStr.c_str(), backtraceLength, backtraceSymbols);

         SAFE_FREE(backtraceSymbols);
      }

      void setContext(const std::string contextStr)
      {
         // note: mind the thread-safety
         // (e.g. don't change the name while multiple threads are accessing the context)
         
         this->contextStr = contextStr;
      }
      

   protected:
      LogContext(Logger* logger, const std::string context) : contextStr(context)
      {
         // for derived classes that have a different way of finding the logger
         // (other than via the thread-local storage)
         // ...and remember to init the context also ;) 
         
         this->logger = logger;
      };
      
      
      // getters & setters
      void setLogger(Logger* logger)
      {
         this->logger = logger;
      }
   

   private:
      std::string contextStr;
      Logger* logger;
   
};

#endif /*LOGCONTEXT_H_*/
