#include <common/Common.h>
#include <common/threading/Thread.h>
#include <common/toolkit/TimeTk.h>
#include <common/threading/Mutex.h>

#include <linux/kthread.h>
#include <linux/sched.h>


void Thread_init(Thread* this, const char* threadName, ThreadRoutine threadRoutine)
{
   this->selfTerminateMutex = Mutex_construct();
   this->shallSelfTerminateCond = Condition_construct();
   this->isSelfTerminatedChangeCond = Condition_construct();

   this->shallSelfTerminate = fhgfs_false;
   this->isSelfTerminated = fhgfs_true; // start with fhgfs_true so that join(..) works with a nerver-ran thread

   StringTk_strncpyTerminated(this->initialThreadName, threadName, BEEGFS_TASK_COMM_LEN);

   this->threadRoutine = threadRoutine;
}

Thread* Thread_construct(const char* threadName, ThreadRoutine threadRoutine)
{
   Thread* this = (Thread*)os_kmalloc(sizeof(*this) );

   Thread_init(this, threadName, threadRoutine);

   return this;
}

void Thread_uninit(Thread* this)
{
   Mutex_destruct(this->selfTerminateMutex);
   Condition_destruct(this->shallSelfTerminateCond);
   Condition_destruct(this->isSelfTerminatedChangeCond);
}

void Thread_destruct(Thread* this)
{
   Thread_uninit(this);

   kfree(this);
}

fhgfs_bool Thread_start(Thread* this)
{
   // set thread to not be selfTerminated
   Mutex_lock(this->selfTerminateMutex);

   this->isSelfTerminated = fhgfs_false;

   Mutex_unlock(this->selfTerminateMutex);

   // actually run the thread
   this->threadData = kthread_run(__Thread_runStatic, this, "%s", this->initialThreadName);

   return !IS_ERR(this->threadData);
}

void Thread_join(Thread* this)
{
   Mutex_lock(this->selfTerminateMutex);

   while(!this->isSelfTerminated)
      Condition_wait(this->isSelfTerminatedChangeCond, this->selfTerminateMutex);

   Mutex_unlock(this->selfTerminateMutex);
}

/**
 * @return fhgfs_true if thread terminated before timeout expired
 */
fhgfs_bool Thread_timedjoin(Thread* this, int timeoutMS)
{
   fhgfs_bool isTerminated;

   Mutex_lock(this->selfTerminateMutex);

   if(!this->isSelfTerminated)
      Condition_timedwait(this->isSelfTerminatedChangeCond, this->selfTerminateMutex, timeoutMS);

   isTerminated = this->isSelfTerminated;

   Mutex_unlock(this->selfTerminateMutex);

   return isTerminated;
}

void Thread_selfTerminate(Thread* this)
{
   Mutex_lock(this->selfTerminateMutex);

   this->shallSelfTerminate = fhgfs_true;
   Condition_broadcast(this->shallSelfTerminateCond);

   Mutex_unlock(this->selfTerminateMutex);
}

void Thread_sleep(int timeoutMS)
{
   long timeout = TimeTk_msToJiffiesSchedulable(timeoutMS);

   set_current_state(TASK_INTERRUPTIBLE);

   timeout = schedule_timeout(timeout);

   __set_current_state(TASK_RUNNING);
}

/**
 * @param data a this-pointer to the Thread struct
 */
int __Thread_runStatic(void* data)
{
   Thread* this = (Thread*)data;

   this->threadRoutine(this);

   __Thread_setSelfTerminated(this);

   return 0;
}

/**
 * Get (comm-)name of the given thread argument.
 */
const char* Thread_getName(Thread* this)
{
   return this->initialThreadName;
}

/**
 * Get comm name of the thread calling this function.
 *
 * @param outBuf must be at least of length BEEGFS_TASK_COMM_LEN
 */
void Thread_getCurrentThreadName(char* outBuf)
{
   StringTk_strncpyTerminated(outBuf, current->comm, BEEGFS_TASK_COMM_LEN);
}

/**
 * Get comm name of the thread calling this function.
 */
const char* Thread_getCurrentThreadNameStr(void)
{
   return current->comm;
}

void Thread_yield(void)
{
   yield();
}
