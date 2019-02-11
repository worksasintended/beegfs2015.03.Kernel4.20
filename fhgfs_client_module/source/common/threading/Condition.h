#ifndef OPEN_CONDITION_H_
#define OPEN_CONDITION_H_

#include <common/Common.h>
#include <common/toolkit/Time.h>
#include <common/toolkit/TimeTk.h>
#include <linux/sched/signal.h> //signal_pending moved here
#include "Mutex.h"

//#include <linux/sched.h>


struct Condition;
typedef struct Condition Condition;

enum cond_wait_res;
typedef enum cond_wait_res cond_wait_res_t;

static inline void Condition_init(Condition* this);
static inline Condition* Condition_construct(void);
static inline void Condition_uninit(Condition* this);
static inline void Condition_destruct(Condition* this);

static inline void Condition_wait(Condition* this, Mutex* mutex);
static inline cond_wait_res_t Condition_waitInterruptible(Condition* this, Mutex* mutex);
static inline cond_wait_res_t Condition_timedwait(Condition* this, Mutex* mutex, int timeoutMS);
static inline cond_wait_res_t Condition_timedwaitInterruptible(Condition* this, Mutex* mutex,
   int timeoutMS);
static inline void Condition_broadcast(Condition* this);
static inline void Condition_signal(Condition* this);


struct Condition
{
   wait_queue_head_t queue;
};


enum cond_wait_res
{
   COND_WAIT_TIMEOUT =  0, // make sure that timeout is always 0 
   COND_WAIT_SUCCESS =  1,
   COND_WAIT_SIGNAL  = -1
};


void Condition_init(Condition* this)
{
   init_waitqueue_head(&this->queue);
}

Condition* Condition_construct(void)
{
   Condition* this = (Condition*)os_kmalloc(sizeof(*this) );

   if(likely(this) )
      Condition_init(this);

   return this;
}

void Condition_uninit(Condition* this)
{
}

void Condition_destruct(Condition* this)
{
   Condition_uninit(this);

   kfree(this);
}

/**
 * Note: Waits uninterruptible.
 *
 * Uninterruptible waits are important in cases where we cannot continue until a certain external
 * condition becomes true. Otherwise we might go up to 100% cpu usage on ctrl+c because schedule()
 * will keep waking up immediately.
 *
 * The problem with uninterruptible waiting is that it will produce hung_task log msgs in the
 * kernel, so it is not appropriate for long sleeps like a worker waiting for new work.
 */
void Condition_wait(Condition* this, Mutex* mutex)
{
   wait_queue_entry_t wait;

   init_waitqueue_entry(&wait, current);
   add_wait_queue(&this->queue, &wait);

   set_current_state(TASK_UNINTERRUPTIBLE);

   Mutex_unlock(mutex);

   schedule();

   Mutex_lock(mutex);

   remove_wait_queue(&this->queue, &wait);

   __set_current_state(TASK_RUNNING);
}

/**
 * Note: We have this, because:
 * a) Some threads explicitly want to be interruptible, e.g. our statfs want's to know about
 * ctrl+c to raise the work interrupt flag and afterwards waits uninterruptible.
 * b) Long unterruptible waiting produces hung_task msgs in the kernel log, so interruptible wait
 * should be used e.g. with workers waiting for new work.
 *
 * @return COND_WAIT_SIGNAL if interrupted by signal, COND_WAIT_SUCCESS otherwise
 */
cond_wait_res_t Condition_waitInterruptible(Condition* this, Mutex* mutex)
{
   cond_wait_res_t retVal;
   wait_queue_entry_t wait;

   init_waitqueue_entry(&wait, current);
   add_wait_queue(&this->queue, &wait);

   set_current_state(TASK_INTERRUPTIBLE);

   Mutex_unlock(mutex);

   schedule();

   retVal = signal_pending(current) ? COND_WAIT_SIGNAL : COND_WAIT_SUCCESS;

   Mutex_lock(mutex);

   remove_wait_queue(&this->queue, &wait);

   __set_current_state(TASK_RUNNING);

   return retVal;
}

/**
 * Note: Waits uninterruptible. Read the _wait() comments.
 *
 * @return COND_WAIT_TIMEOUT if timeout occurred
 */
cond_wait_res_t Condition_timedwait(Condition* this, Mutex* mutex, int timeoutMS)
{
   cond_wait_res_t retVal;
   long __timeout;

wait_queue_entry_t wait;
   init_waitqueue_entry(&wait, current);
   add_wait_queue(&this->queue, &wait);

   set_current_state(TASK_UNINTERRUPTIBLE);

   Mutex_unlock(mutex);

   // timeval to jiffies
   __timeout = TimeTk_msToJiffiesSchedulable(timeoutMS);

   // wait
   if( (__timeout = schedule_timeout(__timeout) ) )
      retVal = COND_WAIT_SUCCESS;
   else
      retVal = COND_WAIT_TIMEOUT;

   Mutex_lock(mutex);

   remove_wait_queue(&this->queue, &wait);

   __set_current_state(TASK_RUNNING);

   return retVal;
}

/**
 * Note: Waits interruptible. Read the _waitInterruptible() comments.
 *
 * @return COND_WAIT_TIMEOUT if timeout occurred, COND_WAIT_SIGNAL on pending signal.
 */
cond_wait_res_t Condition_timedwaitInterruptible(Condition* this, Mutex* mutex, int timeoutMS)
{
   cond_wait_res_t retVal;
   long __timeout;

   wait_queue_entry_t wait;
   init_waitqueue_entry(&wait, current);
   add_wait_queue(&this->queue, &wait);

   set_current_state(TASK_INTERRUPTIBLE);

   Mutex_unlock(mutex);

   // timeval to jiffies
   __timeout = TimeTk_msToJiffiesSchedulable(timeoutMS);

   // wait
   if( (__timeout = schedule_timeout(__timeout) ) )
      retVal = signal_pending(current) ? COND_WAIT_SIGNAL : COND_WAIT_SUCCESS;
   else
      retVal = COND_WAIT_TIMEOUT;

   Mutex_lock(mutex);

   remove_wait_queue(&this->queue, &wait);

   __set_current_state(TASK_RUNNING);

   return retVal;
}

void Condition_broadcast(Condition* this)
{
   wake_up_all(&this->queue);
}

void Condition_signal(Condition* this)
{
   wake_up(&this->queue);
}


#endif /*OPEN_CONDITION_H_*/
