#include <common/app/log/LogContext.h>
#include <common/threading/RWLock.h>
#include <common/toolkit/Random.h>
#include <common/toolkit/StringTk.h>
#include <common/toolkit/Time.h>
#include "TestRWLock.h"


TestRWLock::TestRWLock()
{
   log.setContext("TestRWLock");
}

TestRWLock::~TestRWLock()
{
}

/*
 * will be executed before every test
 * nothing to do
 */
void TestRWLock::setUp()
{
}

/*
 * will be executed after every test
 * nothing to do
 */
void TestRWLock::tearDown()
{
}

/*
 * sorts all threads by the lock timestamp, make it easier to verify the execution order
 */
void TestRWLock::sortThreadsInLockTimestamp(TestLockThread* threads,
   Time* startTime)
{
   bool changesDone = false;

   do
   {
      changesDone = false;

      for (int id = 0; id < (TestRWLock_THREAD_COUNT -1); id++)
      {
         int elapsedTimeID = threads[id].getLockTimestamp().
            elapsedSinceMS(startTime);
         int elapsedTimeNextID = threads[id + 1].getLockTimestamp().
            elapsedSinceMS(startTime);

         if (elapsedTimeID > elapsedTimeNextID)
         {
            TestLockThread tmpTestLockThread;
            tmpTestLockThread.copy(&threads[id]);
            threads[id].copy(&threads[id + 1]);
            threads[id + 1 ].copy(&tmpTestLockThread);
            changesDone = true;
         }
      }
   }
   while (changesDone);
}

/*
 * analyze the run time for random thread execution, if the run time was long enough
 */
void TestRWLock::checkRandomRuntime(TestLockThread* threads, int runtimeMS)
{
   int minimalRuntimeMS = 0;
   int longestSleepMS = 0;

   for (int id = 0; id < TestRWLock_THREAD_COUNT; id++)
   {
      // ignore threads which didn't get a lock
      if (!threads[id].getLockSuccess())
      {
         continue;
      }

      if (threads[id].getDoReadLock())
      { // reader thread, find the longest sleep time
         for (int nextId = id + 1; nextId < TestRWLock_THREAD_COUNT; nextId++)
         {
            // check all reader threads which was executed at the same time
            if(threads[nextId].getDoReadLock())
            { // next thread was a reader, find the longest sleep time
               if (threads[nextId].getLockSuccess() &&
                  (longestSleepMS < threads[nextId].getSleepTimeMS()))
               {
                  longestSleepMS = threads[nextId].getSleepTimeMS();
               }
            }
            else
            { // next thread was a writer, stop searching of longest sleep time
               id = nextId - 1;
               break;
            }
         }
         minimalRuntimeMS = minimalRuntimeMS + longestSleepMS;
      }
      else
      { // writer thread, add the the sleep time
         minimalRuntimeMS = minimalRuntimeMS + threads[id].getSleepTimeMS();
      }
   }

   // check if the sleep time is bigger then the runtime
   if (minimalRuntimeMS > runtimeMS)
   {
      CPPUNIT_FAIL("Runtime is to short. A lock didn't work.");
   }
}

/*
 * analyze the execution order for random thread execution
 */
void TestRWLock::checkRandomExecutionOrder(TestLockThread* threads)
{
   for (int id = 1; id < (TestRWLock_THREAD_COUNT); id++)
   {
      // ignore threads which didn't get a lock
      if(!threads[id].getLockSuccess())
      {
         continue;
      }

      // check if the unlock was successful
      if(!threads[id].getUnlockSuccess())
      {
         CPPUNIT_FAIL("Test thread didn't unlock the lock.");
      }

      // check the execution order
      if(threads[id].getDoReadLock())
      {
         checkRandomExecutionOrderReader(threads, id);
      }
      else
      {
         checkRandomExecutionOrderWriter(threads, id);
      }
   }
}

/*
 * analyze the execution order for random thread execution for a reader thread
 */
void TestRWLock::checkRandomExecutionOrderReader(TestLockThread* threads, int threadID)
{
   for (int beforeId = threadID - 1; beforeId >= 0; beforeId--)
   {
      if(threads[beforeId].getLockSuccess())
      {
         Time unlockTimeBefore = threads[beforeId].getUnlockTimestamp();
         Time lockTime = threads[threadID].getLockTimestamp();

         // check if the thread before was a writer and check if the thread before has unlocked
         // the rwlock before this reader thread locks the rwlock
         // if the thread before was a reader it is OK to get the lock without a unlock
         if((!threads[beforeId].getDoReadLock()) && (unlockTimeBefore > lockTime))
         {
            std::cerr << "execution order failed, time diff in micro sec: " << StringTk::uintToStr(
               unlockTimeBefore.elapsedSinceMicro(&lockTime)) << std::endl;
            CPPUNIT_FAIL("Test thread got the lock, but it wasn't possible to get the lock.");
         }
      }
   }
}

/*
 * analyze the execution order for random thread execution for a writer thread
 */
void TestRWLock::checkRandomExecutionOrderWriter(TestLockThread* threads, int threadID)
{
   for (int beforeId = threadID - 1; beforeId >= 0; beforeId--)
   {
      if(threads[beforeId].getLockSuccess())
      {
         Time unlockTimeBefore = threads[beforeId].getUnlockTimestamp();
         Time lockTime = threads[threadID].getLockTimestamp();

         // check if the thread before has unlocked the rwlock before this writer thread locks
         // the rwlock
         if(unlockTimeBefore > lockTime)
         {
            std::cerr << "execution order failed, time diff in micro sec: " << StringTk::uintToStr(
               unlockTimeBefore.elapsedSinceMicro(&lockTime)) << std::endl;
            CPPUNIT_FAIL("Test thread got the lock, but it wasn't possible to get the lock.");
         }
      }
   }
}

/*
 * tests a reader thread on a read lock, checks the basic functions of a rwlock
 */
void TestRWLock::testReaderOnReader()
{
   log.log(Log_DEBUG, "testReaderOnReader started");

   // creates a read lock
   RWLock lock;
   lock.readLock();

   Time startTime;

   // creates a read lock
   TestLockThread thread(&lock, true, false);
   thread.start();

   // wait a few second before unlock the lock
   PThread::sleepMS(TestRWLock_SLEEP_TIME_MS);

   lock.unlock();

   // wait for timeout
   bool notTimedOut = thread.timedjoin(TestRWLock_SINGLE_TIMEOUT_MS);
   int runtime = startTime.elapsedMS();

   if (!notTimedOut)
   {
      CPPUNIT_FAIL("Test ran into a timeout. Maybe a dead-lock");
   }

   if (!thread.getLockSuccess())
   {
      CPPUNIT_FAIL("The test thread didn't get the lock, but it was possible "
         "to get the lock.");
   }

   if (!thread.getUnlockSuccess())
   {
      CPPUNIT_FAIL("The test thread didn't unlock the lock.");
   }

   if (TestRWLock_SLEEP_TIME_MS > runtime)
   {
      CPPUNIT_FAIL("Runtime is to short. A lock didn't work.");
   }

   log.log(Log_DEBUG, "testReaderOnReader finished");
}

/*
 * tests a reader thread on a write lock, checks the basic functions of a rwlock
 */
void TestRWLock::testReaderOnWriter()
{
   log.log(Log_DEBUG, "testReaderOnWriter started");

   // creates a write lock
   RWLock lock;
   lock.writeLock();

   Time startTime;

   // creates a read lock
   TestLockThread thread(&lock, true, false);
   thread.start();

   // wait a few second before unlock the lock
   PThread::sleepMS(TestRWLock_SLEEP_TIME_MS);

   lock.unlock();

   // wait for timeout
   bool notTimedOut = thread.timedjoin(2 * TestRWLock_SINGLE_TIMEOUT_MS);
   int runtime = startTime.elapsedMS();

   if (!notTimedOut)
   {
      CPPUNIT_FAIL("Test ran into a timeout. Maybe a dead-lock");
   }

   if (!thread.getLockSuccess())
   {
      CPPUNIT_FAIL("The test thread didn't get the lock, but it was possible "
         "to get the lock.");
   }

   if (!thread.getUnlockSuccess())
   {
      CPPUNIT_FAIL("The test thread didn't unlock the lock.");
   }

   if ((TestRWLock_SLEEP_TIME_MS * 2) > runtime)
   {
      CPPUNIT_FAIL("Runtime is to short. A lock didn't work.");
   }

   log.log(Log_DEBUG, "testReaderOnWriter finished");
}

/*
 * tests a writer thread on a read lock, checks the basic functions of a rwlock
 */
void TestRWLock::testWriterOnReader()
{
   log.log(Log_DEBUG, "testWriterOnReader started");

   // creates a read lock
   RWLock lock;
   lock.readLock();

   Time startTime;

   // creates a write lock
   TestLockThread thread(&lock, false, false);
   thread.start();

   // wait a few second before unlock the lock
   PThread::sleepMS(TestRWLock_SLEEP_TIME_MS);

   lock.unlock();

   // wait for timeout
   bool notTimedOut = thread.timedjoin(2 * TestRWLock_SINGLE_TIMEOUT_MS);
   int runtime = startTime.elapsedMS();

   if (!notTimedOut)
   {
      CPPUNIT_FAIL("Test ran into a timeout. Maybe a dead-lock");
   }

   if (!thread.getLockSuccess())
   {
      CPPUNIT_FAIL("The test thread didn't get the lock, but it was possible "
         "to get the lock.");
   }

   if (!thread.getUnlockSuccess())
   {
      CPPUNIT_FAIL("The test thread didn't unlock the lock.");
   }

   if ((TestRWLock_SLEEP_TIME_MS * 2) > runtime)
   {
      CPPUNIT_FAIL("Runtime is to short. A lock didn't work.");
   }

   log.log(Log_DEBUG, "testWriterOnReader finished");
}

/*
 * tests a writer thread on a write lock, checks the basic functions of a rwlock
 */
void TestRWLock::testWriterOnWriter()
{
   log.log(Log_DEBUG, "testWriterOnWriter started");

   // creates a write lock
   RWLock lock;
   lock.writeLock();

   Time startTime;

   // creates a write lock
   TestLockThread thread(&lock, false, false);
   thread.start();

   // wait a few second before unlock the lock
   PThread::sleepMS(TestRWLock_SLEEP_TIME_MS);

   lock.unlock();

   // wait for timeout
   bool notTimedOut = thread.timedjoin(2 * TestRWLock_SINGLE_TIMEOUT_MS);
   int runtime = startTime.elapsedMS();

   if (!notTimedOut)
   {
      CPPUNIT_FAIL("Test ran into a timeout. Maybe a dead-lock");
   }

   if (!thread.getLockSuccess())
   {
      CPPUNIT_FAIL("The test thread didn't get the lock, but it was possible "
         "to get the lock.");
   }

   if (!thread.getUnlockSuccess())
   {
      CPPUNIT_FAIL("The test thread didn't unlock the lock.");
   }

   if ((TestRWLock_SLEEP_TIME_MS * 2) > runtime)
   {
      CPPUNIT_FAIL("Runtime is to short. A lock didn't work.");
   }

   log.log(Log_DEBUG, "testWriterOnWriter finished");
}

/*
 * tests reader threads and writer thread on a read lock, checks massive amount of lock operations
 */
void TestRWLock::testRandomOnReader()
{
   log.log(Log_DEBUG, "testRandomOnReader started");

   RWLock lock;
   Random randomizer;
   TestLockThread threadList[TestRWLock_THREAD_COUNT];

   int id = 0;

   threadList[0].init(&lock, true, false, TestRWLock_SLEEP_TIME_MS, 1);

   // create all threads for the test
   for (id = 1; id < TestRWLock_THREAD_COUNT; id++)
   {
      bool tmpDoReadLock;

      // random initialization: is reader or writer
      if (randomizer.getNextInRange(0, 1) == 1)
         tmpDoReadLock = true;
      else
         tmpDoReadLock = false;

      // random initialization: sleep time
      int tmpSleepTimeMS = randomizer.getNextInRange(
         TestRWLock_RANDOM_SLEEP_TIME_MIN_MS,
         TestRWLock_RANDOM_SLEEP_TIME_MAX_MS);

      // random initialization: start delay time
      int tmpLockDelayMS = randomizer.getNextInRange(
         TestRWLock_RANDOM_LOCK_DELAY_MIN_MS,
         TestRWLock_RANDOM_LOCK_DELAY_MAX_MS);

      threadList[id].init(&lock, tmpDoReadLock, false, tmpSleepTimeMS, tmpLockDelayMS);
   }

   Time startTime;

   // start all threads
   threadList[0].start();
   for (id = 1; id < TestRWLock_THREAD_COUNT; id++)
   {
      threadList[id].start();
   }

   bool notTimedOut = true;
   Time startTimeout;

   // collect all threads and check timeout
   for (id = 0; id < TestRWLock_THREAD_COUNT; id++)
   {
      int nextTimeout = TestRWLock_MULTI_TIMEOUT_MS - startTimeout.elapsedMS();
      if (nextTimeout < 500)
      {
         nextTimeout = 500;
      }

      if (!threadList[id].timedjoin(nextTimeout))
      {
         notTimedOut = false;
      }
   }

   int runtimeMS = startTime.elapsedMS();

   // check the constraints
   if (!notTimedOut)
   {
      CPPUNIT_FAIL("Test ran into a timeout. Maybe a dead-lock");
   }

   sortThreadsInLockTimestamp(threadList, &startTime);
   checkRandomExecutionOrder(threadList);
   checkRandomRuntime(threadList, runtimeMS);

   log.log(Log_DEBUG, "testRandomOnReader finished");
}

/*
 * tests reader threads and writer thread on a write lock, checks massive amount of lock operations
 */
void TestRWLock::testRandomOnWriter()
{
   log.log(Log_DEBUG, "testRandomOnWriter started");

   RWLock lock;
   Random randomizer;
   TestLockThread threadList[TestRWLock_THREAD_COUNT];

   int id = 0;

   threadList[0].init(&lock, false, false, 5000, 1);

   // create all threads for the test
   for (id = 1; id < TestRWLock_THREAD_COUNT; id++)
   {
      bool tmpDoReadLock;

      // random initialization: is reader or writer
      if (randomizer.getNextInRange(0, 1) == 1)
         tmpDoReadLock = true;
      else
         tmpDoReadLock = false;

      // random initialization: sleep time
      int tmpSleepTimeMS = randomizer.getNextInRange(
         TestRWLock_RANDOM_SLEEP_TIME_MIN_MS,
         TestRWLock_RANDOM_SLEEP_TIME_MAX_MS);

      // random initialization: start delay time
      int tmpLockDelayMS = randomizer.getNextInRange(
         TestRWLock_RANDOM_LOCK_DELAY_MIN_MS,
         TestRWLock_RANDOM_LOCK_DELAY_MAX_MS);

      threadList[id].init(&lock, tmpDoReadLock, false, tmpSleepTimeMS, tmpLockDelayMS);
   }

   Time startTime;

   // start all threads
   threadList[0].start();
   for (id = 1; id < TestRWLock_THREAD_COUNT; id++)
   {
      threadList[id].start();
   }

   bool notTimedOut = true;
   Time startTimeout;

   // collect all threads and check timeout
   for (id = 0; id < TestRWLock_THREAD_COUNT; id++)
   {
      int nextTimeout = TestRWLock_MULTI_TIMEOUT_MS - startTimeout.elapsedMS();
      if (nextTimeout < 500)
      {
         nextTimeout = 500;
      }

      if (!threadList[id].timedjoin(nextTimeout))
      {
         notTimedOut = false;
      }
   }

   int runtimeMS = startTime.elapsedMS();

   // check the constraints
   if (!notTimedOut)
   {
      CPPUNIT_FAIL("Test ran into a timeout. Maybe a dead-lock");
   }

   sortThreadsInLockTimestamp(threadList, &startTime);
   checkRandomExecutionOrder(threadList);
   checkRandomRuntime(threadList, runtimeMS);

   log.log(Log_DEBUG, "testRandomOnWriter finished");
}

/*
 * tests a tryReadLock on a read lock, checks the basic functions of a rwlock
 */
void TestRWLock::testTryReadOnReader()
{
   log.log(Log_DEBUG, "testTryReadOnReader started");

   // creates a read lock with try method
   RWLock lock;

   bool success = lock.tryReadLock();
   if (!success)
   {
      CPPUNIT_FAIL("Couldn't get initial lock.");
   }

   Time startTime;

   // creates a read lock with try method
   TestLockThread thread(&lock, true, true);
   thread.start();

   // wait a few second before unlock the lock
   PThread::sleepMS(TestRWLock_SLEEP_TIME_MS);

   lock.unlock();

   // wait for timeout
   bool notTimedOut = thread.timedjoin(TestRWLock_SINGLE_TIMEOUT_MS);
   int runtime = startTime.elapsedMS();

   if (!notTimedOut)
   {
      CPPUNIT_FAIL("Test ran into a timeout. Maybe a dead-lock");
   }

   if (!thread.getLockSuccess())
   {
      CPPUNIT_FAIL("The test thread didn't get the lock, but it was possible "
         "to get the lock.");
   }

   if (!thread.getUnlockSuccess())
   {
      CPPUNIT_FAIL("The test thread didn't unlock the lock.");
   }

   if (TestRWLock_SLEEP_TIME_MS > runtime)
   {
      CPPUNIT_FAIL("Runtime is to short. A lock didn't work.");
   }

   log.log(Log_DEBUG, "testTryReadOnReader finished");
}

/*
 * tests a tryReadLock on a write lock, checks the basic functions of a rwlock
 */
void TestRWLock::testTryReadOnWriter()
{
   log.log(Log_DEBUG, "testTryReadOnWriter started");

   // creates a write lock with try method
   RWLock lock;

   bool success = lock.tryWriteLock();
   if (!success)
   {
      CPPUNIT_FAIL("Couldn't get initial lock.");
   }

   Time startTime;

   // creates a read lock with try method
   TestLockThread thread(&lock, true, true);
   thread.start();

   // wait a few second before unlock the lock
   PThread::sleepMS(TestRWLock_SLEEP_TIME_MS);

   lock.unlock();

   // wait for timeout
   bool notTimedOut = thread.timedjoin(2 * TestRWLock_SINGLE_TIMEOUT_MS);
   int runtime = startTime.elapsedMS();

   if (!notTimedOut)
   {
      CPPUNIT_FAIL("Test ran into a timeout. Maybe a dead-lock");
   }

   if (thread.getLockSuccess())
   {
      CPPUNIT_FAIL("The test thread got the lock, but it wasn't possible to "
         "get the lock.");
   }

   if ((TestRWLock_SLEEP_TIME_MS) > runtime)
   {
      CPPUNIT_FAIL("Runtime is to short. A lock didn't work.");
   }

   log.log(Log_DEBUG, "testTryReadOnWriter finished");
}

/*
 * tests a tryWriteLock on a read lock, checks the basic functions of a rwlock
 */
void TestRWLock::testTryWriteOnReader()
{
   log.log(Log_DEBUG, "testTryWriteOnReader started");

   // creates a read lock with try method
   RWLock lock;

   bool success = lock.tryReadLock();
   if (!success)
   {
      CPPUNIT_FAIL("Couldn't get initial lock.");
   }

   Time startTime;

   // creates a write lock with try method
   TestLockThread thread(&lock, false, true);
   thread.start();

   // wait a few second before unlock the lock
   PThread::sleepMS(TestRWLock_SLEEP_TIME_MS);

   lock.unlock();

   // wait for timeout
   bool notTimedOut = thread.timedjoin(2 * TestRWLock_SINGLE_TIMEOUT_MS);
   int runtime = startTime.elapsedMS();

   if (!notTimedOut)
   {
      CPPUNIT_FAIL("Test ran into a timeout. Maybe a dead-lock");
   }

   if (thread.getLockSuccess())
   {
      CPPUNIT_FAIL("The test thread got the lock, but it wasn't possible to "
         "get the lock.");
   }

   if ((TestRWLock_SLEEP_TIME_MS) > runtime)
   {
      CPPUNIT_FAIL("Runtime is to short. A lock didn't work.");
   }

   log.log(Log_DEBUG, "testTryWriteOnReader finished");
}

/*
 * tests a tryWriteLock on a write lock, checks the basic functions of a rwlock
 */
void TestRWLock::testTryWriteOnWriter()
{
   log.log(Log_DEBUG, "testTryWriteOnWriter started");

   // creates a write lock with try method
   RWLock lock;

   bool success = lock.tryWriteLock();
   if (!success)
   {
      CPPUNIT_FAIL("Couldn't get initial lock.");
   }

   Time startTime;

   // creates a write lock with try method
   TestLockThread thread(&lock, false, true);
   thread.start();

   // wait a few second before unlock the lock
   PThread::sleepMS(TestRWLock_SLEEP_TIME_MS);

   lock.unlock();

   // wait for timeout
   bool notTimedOut = thread.timedjoin(2 * TestRWLock_SINGLE_TIMEOUT_MS);
   int runtime = startTime.elapsedMS();

   if (!notTimedOut)
   {
      CPPUNIT_FAIL("Test ran into a timeout. Maybe a dead-lock");
   }

   if (thread.getLockSuccess())
   {
      CPPUNIT_FAIL("The test thread got the lock, but it wasn't possible to "
         "get the lock.");
   }

   if ((TestRWLock_SLEEP_TIME_MS) > runtime)
   {
      CPPUNIT_FAIL("Runtime is to short. A lock didn't work.");
   }

   log.log(Log_DEBUG, "testTryWriteOnWriter finished");
}

/*
 * tests tryReadLock and tryWriteLock on a read lock, checks massive amount of lock operations
 */
void TestRWLock::testRandomTryOnReader()
{
   log.log(Log_DEBUG, "testRandomTryOnReader started");

   RWLock lock;
   Random randomizer;
   TestLockThread threadList[TestRWLock_THREAD_COUNT];

   int id = 0;

   threadList[0].init(&lock, true, true, 5000, 1);

   // create all threads for the test
   for (id = 1; id < TestRWLock_THREAD_COUNT; id++)
   {
      bool tmpDoReadLock;

      // random initialization: is reader or writer
      if (randomizer.getNextInRange(0, 1) == 1)
         tmpDoReadLock = true;
      else
         tmpDoReadLock = false;

      // random initialization: sleep time
      int tmpSleepTimeMS = randomizer.getNextInRange(
         TestRWLock_RANDOM_SLEEP_TIME_MIN_MS,
         TestRWLock_RANDOM_SLEEP_TIME_MAX_MS);

      // random initialization: start delay time
      int tmpLockDelayMS = randomizer.getNextInRange(
         TestRWLock_RANDOM_LOCK_DELAY_MIN_MS,
         TestRWLock_RANDOM_LOCK_DELAY_MAX_MS);

      threadList[id].init(&lock, tmpDoReadLock, true, tmpSleepTimeMS, tmpLockDelayMS);
   }

   Time startTime;

   // start all threads
   threadList[0].start();
   for (id = 1; id < TestRWLock_THREAD_COUNT; id++)
   {
      threadList[id].start();
   }

   bool notTimedOut = true;
   Time startTimeout;

   // collect all threads and check timeout
   for (id = 0; id < TestRWLock_THREAD_COUNT; id++)
   {
      int nextTimeout = TestRWLock_MULTI_TIMEOUT_MS - startTimeout.elapsedMS();
      if (nextTimeout < 500)
      {
         nextTimeout = 500;
      }

      if (!threadList[id].timedjoin(nextTimeout))
      {
         notTimedOut = false;
      }
   }

   int runtimeMS = startTime.elapsedMS();

   // check the constraints
   if (!notTimedOut)
   {
      CPPUNIT_FAIL("Test ran into a timeout. Maybe a dead-lock");
   }

   sortThreadsInLockTimestamp(threadList, &startTime);
   checkRandomExecutionOrder(threadList);
   checkRandomRuntime(threadList, runtimeMS);

   log.log(Log_DEBUG, "testRandomTryOnReader finished");
}

/*
* tests tryReadLock and tryWriteLock on a write lock, checks massive amount of lock operations
 */
void TestRWLock::testRandomTryOnWriter()
{
   log.log(Log_DEBUG, "testRandomTryOnWriter started");

   RWLock lock;
   Random randomizer;
   TestLockThread threadList[TestRWLock_THREAD_COUNT];

   int id = 0;

   threadList[0].init(&lock, false, true, 5000, 1);

   // create all threads for the test
   for (id = 1; id < TestRWLock_THREAD_COUNT; id++)
   {
      bool tmpDoReadLock;

      // random initialization: is reader or writer
      if (randomizer.getNextInRange(0, 1) == 1)
         tmpDoReadLock = true;
      else
         tmpDoReadLock = false;

      // random initialization: sleep time
      int tmpSleepTimeMS = randomizer.getNextInRange(
         TestRWLock_RANDOM_SLEEP_TIME_MIN_MS,
         TestRWLock_RANDOM_SLEEP_TIME_MAX_MS);

      // random initialization: start delay time
      int tmpLockDelayMS = randomizer.getNextInRange(
         TestRWLock_RANDOM_LOCK_DELAY_MIN_MS,
         TestRWLock_RANDOM_LOCK_DELAY_MAX_MS);

      threadList[id].init(&lock, tmpDoReadLock, true, tmpSleepTimeMS, tmpLockDelayMS);
   }

   Time startTime;

   // start all threads
   threadList[0].start();
   for (id = 1; id < TestRWLock_THREAD_COUNT; id++)
   {
      threadList[id].start();
   }

   bool notTimedOut = true;
   Time startTimeout;

   // collect all threads and check timeout
   for (id = 0; id < TestRWLock_THREAD_COUNT; id++)
   {
      int nextTimeout = TestRWLock_MULTI_TIMEOUT_MS - startTimeout.elapsedMS();
      if (nextTimeout < 500)
      {
         nextTimeout = 500;
      }

      if (!threadList[id].timedjoin(nextTimeout))
      {
         notTimedOut = false;
      }
   }

   int runtimeMS = startTime.elapsedMS();

   // check the constraints
   if (!notTimedOut)
   {
      CPPUNIT_FAIL("Test ran into a timeout. Maybe a dead-lock");
   }

   sortThreadsInLockTimestamp(threadList, &startTime);
   checkRandomExecutionOrder(threadList);
   checkRandomRuntime(threadList, runtimeMS);

   log.log(Log_DEBUG, "testRandomTryOnWriter finished");
}
