#include <common/toolkit/StringTk.h>
#include <common/threading/PThread.h>
#include <common/toolkit/TimeAbs.h>
#include "Logger.h"

#include <time.h>

#define LOGGER_ROTATED_FILE_SUFFIX  ".old-"
#define LOGGER_TIMESTR_SIZE         32

// Note: Keep in sync with enum LogTopic
const Logger::LogTopicElem Logger::LogTopics[] =
{
   {"general",           LogTopic_GENERAL},
   {"state-sync",        LogTopic_STATESYNC},
   {"advanced",          LogTopic_ADVANCED},
   {"unknown",           LogTopic_INVALID}
};

Logger::Logger(ICommonConfig* cfg)
{
   int defaultLogLevel = cfg->getLogLevel();

   logLevels.reserve(LogTopic_INVALID);
   for (int i = 0; i < LogTopic_INVALID; i++)
      logLevels.push_back(defaultLogLevel);

   this->logErrsToStdlog = cfg->getLogErrsToStdlog();
   this->logNoDate = cfg->getLogNoDate();
   this->logStdFile = cfg->getLogStdFile();
   this->logErrFile = cfg->getLogErrFile();
   this->logNumLines = cfg->getLogNumLines();
   this->logNumRotatedFiles = cfg->getLogNumRotatedFiles();
   
   this->stdFile = stdout;
   this->errFile = stderr;
   
   // this->currentNumStdLines = 0; atomics are initialized with 0 already
   // this->currentNumErrLines = 0; atomics are initialized with 0 already
   
   this->timeFormat = logNoDate ? "%X" : "%b%d %X"; // set time format
   
   this->rotatedFileSuffix = LOGGER_ROTATED_FILE_SUFFIX;
   
   pthread_rwlockattr_t attr;
   pthread_rwlockattr_init(&attr);
   pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_READER_NP);

   pthread_rwlock_init(&this->rwLock, &attr);

   // prepare file handles and rotate
   prepareLogFiles();
}

Logger::~Logger()
{
   // close files
   if(this->stdFile != stdout)
      fclose(this->stdFile);
      
   if(!this->logErrsToStdlog && (this->errFile != stderr) )
      fclose(this->errFile);
      
   pthread_rwlock_destroy(&this->rwLock);

}

/**
 * Prints a message to the standard log.
 * Note: Doesn't lock the outputLock
 * 
 * @param level the level of relevance (should be greater than 0)
 * @param msg the actual log message
 */
void Logger::logGrantedUnlocked(int level, const char* threadName, const char* context,
   const char* msg)
{
   char timeStr[LOGGER_TIMESTR_SIZE];

   TimeAbs nowTime;

   getTimeStr(nowTime.getTimeS(), timeStr, LOGGER_TIMESTR_SIZE);

#ifdef BEEGFS_DEBUG_PROFILING
   uint64_t timeMicroS = nowTime.getTimeMicroSecPart(); // additional micro-s info for timestamp

   fprintf(stdFile, "(%d) %s.%06ld %s [%s] >> %s\n", level, timeStr, (long) timeMicroS,
      threadName, context, msg);
#else
   fprintf(stdFile, "(%d) %s %s [%s] >> %s\n", level, timeStr, threadName, context, msg);
#endif // BEEGFS_DEBUG_PROFILING

   //fflush(stdFile); // no longer needed => line buf
   
   currentNumStdLines.increase();
}

/**
 * Wrapper for logGrantedUnlocked that locks/unlocks the outputMutex.
 */
void Logger::logGranted(int level, const char* threadName, const char* context, const char* msg)
{
   pthread_rwlock_rdlock(&this->rwLock);

   logGrantedUnlocked(level, threadName, context, msg);

   pthread_rwlock_unlock(&this->rwLock);

   rotateStdLogChecked();
}

/**
 * Prints a message to the error log.
 * 
 * @param msg the actual log message
 */
void Logger::logErrGranted(const char* threadName, const char* context, const char* msg)
{
   pthread_rwlock_rdlock(&this->rwLock);
   
   TimeAbs nowTime;

   char timeStr[LOGGER_TIMESTR_SIZE];
   getTimeStr(nowTime.getTimeS(), timeStr, LOGGER_TIMESTR_SIZE);
      
#ifdef BEEGFS_DEBUG_PROFILING
   uint64_t timeMicroS = nowTime.getTimeMicroSecPart(); // additional ms info for timestamp

   fprintf(errFile, "(E) %s.%06ld %s [%s] >> %s\n", timeStr, (long) timeMicroS,
      threadName, context, msg);
#else
   fprintf(errFile, "(E) %s %s [%s] >> %s\n", timeStr, threadName, context, msg);
#endif // BEEGFS_DEBUG_PROFILING

   //fflush(errFile); // no longer needed => line buf 
   
   currentNumErrLines.increase();
   pthread_rwlock_unlock(&this->rwLock);

   rotateErrLogChecked();
   
}

/**
 * Prints a backtrage to the standard log.
 */ 
void Logger::logBacktraceGranted(const char* context, int backtraceLength, char** backtraceSymbols)
{
   std::string threadName = PThread::getCurrentThreadName();

   pthread_rwlock_rdlock(&this->rwLock);
   
   logGrantedUnlocked(1, threadName.c_str(), context, "Backtrace:");
   
   for(int i=0; i < backtraceLength; i++)
   {
      fprintf(stdFile, "%d: %s\n", i+1, backtraceSymbols[i] );
   }
   
   pthread_rwlock_unlock(&this->rwLock);
}

/*
 * Print time and date to buf.
 *
 * @param buf output buffer
 * @param bufLen length of buf (depends on current locale, at least 32 recommended)
 * @return 0 on error, strLen of buf otherwise
 */
size_t Logger::getTimeStr(uint64_t seconds, char* buf, size_t bufLen)
{
   struct tm nowStruct;
   localtime_r((time_t*) &seconds, &nowStruct);

   size_t strRes = strftime(buf, bufLen, timeFormat, &nowStruct);
   buf[strRes] = 0; // just to make sure - in case the given buf is too small
   return strRes;
}

void Logger::prepareLogFiles() throw(InvalidConfigException)
{
   if(!logStdFile.length() )
      stdFile = stdout;
   else
   {
      rotateLogFile(logStdFile);
      
      stdFile = fopen(logStdFile.c_str(), "w");
      if(!stdFile)
      {
         perror("Logger::openStdLogFile");
         stdFile = stdout;
         throw InvalidConfigException(
            std::string("Unable to create standard log file: ") + logStdFile);
      }
      
   }

   setlinebuf(stdFile);

   if(logErrsToStdlog)
      return;
   
      
   if(!logErrFile.length() )
      errFile = stderr;
   else
   {
      rotateLogFile(logErrFile);
      
      errFile = fopen(logErrFile.c_str(), "w");
      if(!errFile)
      {
         perror("Logger::openErrLogFile");
         errFile = stderr;
         throw InvalidConfigException(
            std::string("Unable to create error log file: ") + logErrFile);
      }
      
   }

   setlinebuf(errFile);
   
}

void Logger::rotateLogFile(std::string filename)
{
   for(int i=logNumRotatedFiles; i > 0; i--)
   {
      std::string oldname = filename +
         ( (i==1) ? "" : (rotatedFileSuffix + StringTk::intToStr(i-1) ) );

      std::string newname = filename +
         rotatedFileSuffix + StringTk::intToStr(i);

      rename(oldname.c_str(), newname.c_str() );
   }
   
}

void Logger::rotateStdLogChecked()
{

   if((stdFile == stdout) || !logNumLines || (currentNumStdLines.read() < logNumLines) )
      return; // nothing to do yet

   pthread_rwlock_wrlock(&this->rwLock);

   if (currentNumStdLines.read() < logNumLines)
   { // we raced with another thread before aquiring the lock
      pthread_rwlock_unlock(&this->rwLock);
      return;
   }

   currentNumStdLines.setZero();

   fclose(stdFile);
      
   rotateLogFile(logStdFile); // the actual rotation

   stdFile = fopen(logStdFile.c_str(), "w");
   if(!stdFile)
   {
      perror("Logger::rotateStdLogChecked");
      stdFile = stdout;
   }

   setlinebuf(stdFile);

   pthread_rwlock_unlock(&this->rwLock);
}

void Logger::rotateErrLogChecked()
{
   if( (errFile == stderr) || !logNumLines || (currentNumErrLines.read() < logNumLines) )
      return; // nothing to do yet

   pthread_rwlock_wrlock(&this->rwLock);

   if (currentNumErrLines.read() < logNumLines)
   { // we raced with another thread before aquiring the lock
      pthread_rwlock_unlock(&this->rwLock);
      return;
   }
      
   currentNumErrLines.setZero();
      
   fclose(errFile);

   rotateLogFile(logErrFile); // the actual rotation

   errFile = fopen(logErrFile.c_str(), "w");
   if(!errFile)
   {
      perror("Logger::rotateErrLogChecked");
      errFile = stderr;
   }

   setlinebuf(errFile);

   pthread_rwlock_unlock(&this->rwLock);
}
