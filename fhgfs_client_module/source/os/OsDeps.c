#include <common/Common.h>
#include <os/OsDeps.h>
#include <common/FhgfsTypes.h>
#include <common/net/sock/NicAddress.h>
#include <common/net/sock/Socket.h>
#include <toolkit/SignalTk.h>
#include <filesystem/FhgfsOps_versions.h>

#include <linux/netdevice.h>
#include <linux/in.h>
#include <linux/inetdevice.h>


#ifdef CONFIG_STACKTRACE
   #include <linux/stacktrace.h>
#endif

#define MAX_STACK_TRACE_CHAIN 16 // number of functions to to save in a stack trace


struct fhgfs_atomic
{
      atomic_t kernelAtomicVar;
};


fhgfs_bool os_checkCompileTimeAssumptions(void)
{
   if(BEEGFS_IFNAMSIZ < IFNAMSIZ)
   {
      printk_fhgfs(KERN_WARNING, "IFNAMSIZ: assumed: %d; actual: %d\n",
         (int)BEEGFS_IFNAMSIZ, (int)IFNAMSIZ);
      return fhgfs_false;
   }

   if(BEEGFS_IFHWADDRLEN < IFHWADDRLEN)
   {
      printk_fhgfs(KERN_WARNING, "IFHWADDRLEN: assumed: %d; actual: %d\n",
         (int)BEEGFS_IFHWADDRLEN, (int)IFHWADDRLEN);
      return fhgfs_false;
   }

   if(!SocketTk_checkCompileTimeAssumptions() )
      return fhgfs_false;

   return fhgfs_true;
}

int os_printk(const char* fmt, ...)
{
   int retVal;
   va_list ap;

   va_start(ap, fmt);

   retVal = vprintk(fmt, ap);

   va_end(ap);

   return retVal;
}

int os_sprintf(char* buf, const char* fmt, ...)
{
   int retVal;
   va_list ap;

   va_start(ap, fmt);

   retVal = vsprintf(buf, fmt, ap);

   va_end(ap);

   return retVal;
}

/**
 * @return number of bytes that would have been written, even if buf size was too small
 * (typically, you would prefer scnprintf() )
 */
int os_snprintf(char* buf, size_t size, const char* fmt, ...)
{
   int retVal;
   va_list ap;

   va_start(ap, fmt);

   retVal = vsnprintf(buf, size, fmt, ap);

   va_end(ap);

   return retVal;
}

int os_vsnprintf(char* buf, size_t size, const char* fmt, va_list args)
{
   return vsnprintf(buf, size, fmt, args);
}

/**
 * @return number of bytes that actually have been written (always <= size), not including the
 * trailing 0.
 */
int os_scnprintf(char* buf, size_t size, const char* fmt, ...)
{
   int retVal;
   va_list ap;

   va_start(ap, fmt);

   retVal = vscnprintf(buf, size, fmt, ap);

   va_end(ap);

   return retVal;
}

/**
 * @return number of successfully converted elements
 */
int os_sscanf(const char* buf, const char* fmt, ...)
{
   int retVal;
   va_list ap;

   va_start(ap, fmt);

   retVal = vsscanf(buf, fmt, ap);

   va_end(ap);

   return retVal;
}


#ifdef BEEGFS_DEBUG

#if defined CONFIG_STACKTRACE // kernel has stacktrace support

/**
 * Save a given trace. NOTE: Allocated memory has to be freed later on!
 */
void* os_saveStackTrace(void)
{
   struct stack_trace* trace;
   unsigned long *entries;
   
   trace = kmalloc(sizeof(struct stack_trace), GFP_NOFS);
   if (!trace)
      return NULL; // out of memory?

   entries = kmalloc(MAX_STACK_TRACE_CHAIN * sizeof(*entries), GFP_NOFS);
   if (!entries)
   { // out of memory?
      kfree(trace);
      return NULL;
   }

   trace->nr_entries = 0;
   trace->max_entries = MAX_STACK_TRACE_CHAIN;
   trace->entries = entries;
   trace->skip = 1; // cut off ourself, so 1

   save_stack_trace(trace);

   return trace;
}

void os_freeStackTrace(void *trace)
{
   struct stack_trace* os_trace = (struct stack_trace*)trace;

   if (!trace)
   {  // May be NULL, if kmalloc or vmalloc failed
      return;
   }

   kfree(os_trace->entries);
   kfree(os_trace);
}

/**
 * Print a stack trace
 * 
 * @param trace    The stack trace to print
 * @param spaces   Insert 'spaces' white-spaces at the beginning of the line
 */
void os_printStackTrace(void* trace, int spaces)
{
   if (!trace)
   {  // Maybe NULL, if kmalloc or vmalloc failed
      return;
   }

   print_stack_trace((struct stack_trace *)trace, spaces);
}


#else // no CONFIG_STACKTRACE => nothing to do at all

void* os_saveStackTrace(void)
{
   return NULL;
}

void os_printStackTrace(void* trace, int spaces)
{
   printk_fhgfs(KERN_INFO, "Kernel without stack trace support!\n");
   return;
}

void os_freeStackTrace(void* trace)
{
   return;
}

#endif // CONFIG_STACKTRACE

#endif // BEEGFS_DEBUG


