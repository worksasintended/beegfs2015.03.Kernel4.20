#include <common/toolkit/SocketTk.h>
#include <common/net/sock/StandardSocket.h>
#include <common/net/sock/RDMASocket.h>
#include <toolkit/ExternalHelperd.h>
#include <common/Common.h>
#include <common/toolkit/TimeTk.h>

#include <linux/inet.h>
#include <linux/poll.h>
#include <linux/net.h>
#include <linux/socket.h>


struct file* SocketTkDummyFilp = NULL;


/**
 * One-time initialization of a dummy file pointer (required for polling)
 * Note: remember to call the corresponding uninit routine
 */
fhgfs_bool SocketTk_initOnce(void)
{
   SocketTkDummyFilp = filp_open("/dev/null", O_RDONLY, 0);
   if(IS_ERR(SocketTkDummyFilp) )
   {
      printk_fhgfs(KERN_WARNING, "Failed to open the dummy filp for polling\n");
      return fhgfs_false;
   }

   return fhgfs_true;
}

void SocketTk_uninitOnce(void)
{
   if(SocketTkDummyFilp && !IS_ERR(SocketTkDummyFilp) )
      filp_close(SocketTkDummyFilp, NULL);
}


/*
 * Wait for interesting events on a single socket.
 *
 * @param sock wait for incoming data on this socket
 * @return number of socks for which data is available or negative linux error code.
 *  (tvp is being set to time remaining)
 */
int SocketTk_pollSingle(PollSocket* pollSock, int timeoutMS)
{
   struct poll_wqueues table;
   poll_table *wait;
   int table_err = 0;
   int data_available = 0; // return value
   unsigned long mask; // event mask
   struct socket* sock = StandardSocket_getRawSock(pollSock->sock);

   //long __timeout = TimeTk_timevalToJiffiesSchedulable(tvp);
   long __timeout = TimeTk_msToJiffiesSchedulable(timeoutMS);

   poll_initwait(&table);
   wait = &table.pt;

   if(!__timeout)
      wait = NULL;

   pollSock->revents = 0;

   // 1st loop: register the socks that we're waiting for and wait blocking
   // if no data is available yet
   // 2nd loop (after event or timeout): check all socks for available data
   // 3rd and futher loops: in case an uninteresting event occurred
   for( ; ; )
   {
      set_current_state(TASK_INTERRUPTIBLE);

      // ask for available data and register waitqueue

      mask = (*sock->ops->poll)(SocketTkDummyFilp, sock, wait);

      if(mask & (pollSock->events | POLLERR | POLLHUP | POLLNVAL) )
      { // data is available
         data_available = 1;
         pollSock->revents = mask;
      }

      //cond_resched(); // (commented out for latency reasons)


      wait = NULL; // don't register for waiting in the second loop

      if(data_available || !__timeout || signal_pending(current) )
         break;

      if(unlikely(table.error) )
      {
         table_err = table.error;
         break;
      }

      __timeout = schedule_timeout(__timeout);
   }

   __set_current_state(TASK_RUNNING);

   poll_freewait(&table);

   // set remaining time
   //TimeTk_jiffiesToTimeval(tvp, __timeout);

   return table_err ? table_err : data_available;
}


/*
 * Synchronous I/O multiplexing for standard and RDMA sockets.
 * Note: comparable to userspace poll()
 *
 * @param pollSocks wait for interesting events on these socks
 * @param numPollSocks number of pollSocks
 * @return number of socks for which interesting events are available or negative linux error code.
 *  (tvp is being set to time remaining)
 */
int SocketTk_pollEx(PollSocketEx* pollSocks, size_t numPollSocks, int timeoutMS)
{
   struct poll_wqueues stdTable;
   poll_table* stdWait = NULL; // value NULL means "don't register for waiting"
   int table_err = 0;
   unsigned i;
   int numSocksWithREvents = 0; // the return value

   long __timeout = TimeTk_msToJiffiesSchedulable(timeoutMS);

   poll_initwait(&stdTable);

   if(__timeout)
      stdWait = &stdTable.pt;

   // 1st loop: register the socks that we're waiting for and wait blocking
   // if no data is available yet
   // 2nd loop (after event or timeout): check all socks for available data
   //    note: std socks return all events, even those we haven't been waiting for
   // 3rd and futher loops: in case an uninteresting event occurred
   for( ; ; )
   {
      numSocksWithREvents = 0; // (must be inside the loop to be consistent)

      set_current_state(TASK_INTERRUPTIBLE);

      // for each sock: ask for available data and register waitqueue
      for(i = 0; i < numPollSocks; i++)
      {
         PollSocketEx* currentPollSock = &(pollSocks[i] );

         if(Socket_getSockType(currentPollSock->sock) == NICADDRTYPE_RDMA)
         { // RDMA socket
            struct RDMASocket* currentRDMASock = (RDMASocket*)currentPollSock->sock;
            fhgfs_bool finishPoll = (numSocksWithREvents || !__timeout);

            unsigned long mask = RDMASocket_poll(
               currentRDMASock, currentPollSock->events, finishPoll);

            if(mask)
            { // interesting event occurred
               currentPollSock->revents = mask; // save event mask as revents
               numSocksWithREvents++;
            }
         }
         else
         { // Standard socket
            struct socket* currentRawSock =
               StandardSocket_getRawSock( (StandardSocket*)currentPollSock->sock);
            poll_table* currentStdWait = numSocksWithREvents ? NULL : stdWait;

            unsigned long mask = (*currentRawSock->ops->poll)(
               SocketTkDummyFilp, currentRawSock, currentStdWait);

            if(mask & (currentPollSock->events | POLLERR | POLLHUP | POLLNVAL) )
            { // interesting event occurred
               currentPollSock->revents = mask; // save event mask as revents
               numSocksWithREvents++;
            }

            //cond_resched(); // (commented out for latency reasons)
         }
      } // end of for_each_socket loop

      stdWait = NULL; // don't register standard socks for waiting in following loops

      // skip the waiting if we already found something
      if(numSocksWithREvents || !__timeout || signal_pending(current) )
         break;

      // skip the waiting if we have an error
      if(unlikely(stdTable.error) )
      {
         table_err = stdTable.error;
         break;
      }

      // wait (and reduce remaining timeout)
      __timeout = schedule_timeout(__timeout);

   } // end of sleep loop

   __set_current_state(TASK_RUNNING);

   // cleanup loop for RDMA socks
   for(i = 0; i < numPollSocks; i++)
   {
      PollSocketEx* currentPollSock = &(pollSocks[i] );
      if(Socket_getSockType(currentPollSock->sock) == NICADDRTYPE_RDMA)
      {
         struct RDMASocket* currentRDMASock = (RDMASocket*)currentPollSock->sock;
         RDMASocket_poll(currentRDMASock, currentPollSock->events, fhgfs_true);
      }
   }

   // cleanup for standard socks
   poll_freewait(&stdTable);

   //printk_fhgfs(KERN_INFO, "%s:%d: return %d\n", __func__, __LINE__,
   //   table_err ? table_err : numSocksWithREvents); // debug in

   return table_err ? table_err : numSocksWithREvents;
}

/*
 * Synchronous I/O multiplexing.
 * Note: comparable to userspace poll()
 *
 * @param pollSocks wait for interesting events on these socks
 * @param numPollSocks number of pollSocks
 * @return number of socks for which interesting events are available or negative linux error code.
 *  (tvp is being set to time remaining)
 */
int SocketTk_poll(PollSocket* pollSocks, size_t numPollSocks, int timeoutMS)
{
   struct poll_wqueues table;
   poll_table *wait;
   int table_err = 0;
   unsigned i;
   unsigned long mask; // event mask
   int numSocksWithREvents = 0; // the return value
   struct socket* currentSock;
   PollSocket* currentPollSock;

   long __timeout = TimeTk_msToJiffiesSchedulable(timeoutMS);

   poll_initwait(&table);
   wait = &table.pt;

   if(!__timeout)
      wait = NULL;

   // 1st loop: register the socks that we're waiting for and wait blocking
   // if no data is available yet
   // 2nd loop (after event or timeout): check all socks for available data
   // 3rd and futher loops: in case an uninteresting event occurred
   for( ; ; )
   {
      numSocksWithREvents = 0; // (must be inside the loop to be consistent)

      set_current_state(TASK_INTERRUPTIBLE);

      // for each sock: ask for available data and register waitqueue
      for(i = 0; i < numPollSocks; i++)
      {
         currentPollSock = &(pollSocks[i]);
         currentSock = StandardSocket_getRawSock(currentPollSock->sock);

         mask = (*currentSock->ops->poll)(SocketTkDummyFilp, currentSock, wait);

         if(mask & (currentPollSock->events | POLLERR | POLLHUP | POLLNVAL) )
         { // interesting events occurred
            currentPollSock->revents = mask; // save event mask as revents
            wait = NULL; // don't register any more socks for waiting
            numSocksWithREvents++;
         }

         //cond_resched(); // (commented out for latency reasons)
      }

      wait = NULL; // don't register for waiting in following loops

      // skip the rest if we already found something
      if(numSocksWithREvents || !__timeout || signal_pending(current) )
         break;

      // skip the rest if we have an error
      if(table.error)
      {
         table_err = table.error;
         break;
      }

      // wait (and reduce remaining timeout)
      __timeout = schedule_timeout(__timeout);
   }

   __set_current_state(TASK_RUNNING);

   poll_freewait(&table);

   // set remaining time
   //TimeTk_jiffiesToTimeval(tvp, __timeout);

   return table_err ? table_err : numSocksWithREvents;
}


/**
 * Note: Name resolution is performed by asking the helper daemon.
 */
fhgfs_bool SocketTk_getHostByName(struct ExternalHelperd* helperd, const char* hostname,
   fhgfs_in_addr* outIPAddr)
{
   fhgfs_bool retVal = fhgfs_true;
   char* resolveRes;

   resolveRes = ExternalHelperd_getHostByName(helperd, hostname);

   if(!resolveRes)
   { // communication with helper daemon failed => maybe hostname is an IP string
      return SocketTk_getHostByAddrStr(hostname, outIPAddr);
   }

   if(!strlen(resolveRes) )
      return fhgfs_false; // hostname unknown


   // we got an IP string

   retVal = SocketTk_getHostByAddrStr(resolveRes, outIPAddr);


   // clean-up
   kfree(resolveRes);

   return retVal;
}

/**
 * Note: Old kernel versions do not support validation of the IP string.
 *
 * @param outIPAddr set to BEEGFS_INADDR_NONE if an error was detected
 * @return fhgfs_false for wrong input on modern kernels (>= 2.6.20), old kernels always return
 * fhgfs_true
 */
fhgfs_bool SocketTk_getHostByAddrStr(const char* hostAddr, fhgfs_in_addr* outIPAddr)
{

#ifndef KERNEL_HAS_IN4_PTON

   *outIPAddr = in_aton(hostAddr); // this version has no error handling

#else

   if(unlikely(!in4_pton(hostAddr, strlen(hostAddr), (u8 *)outIPAddr, -1, NULL) ) )
   { // not a valid address string
      *outIPAddr = BEEGFS_INADDR_NONE;
      return fhgfs_false;
   }

#endif // LINUX_VERSION_CODE

   return fhgfs_true;
}

fhgfs_bool SocketTk_checkCompileTimeAssumptions(void)
{
   if( (BEEGFS_POLLIN != POLLIN) ||
      (BEEGFS_POLLPRI != POLLPRI) ||
      (BEEGFS_POLLOUT != POLLOUT) ||
      (BEEGFS_POLLERR != POLLERR) ||
      (BEEGFS_POLLHUP != POLLHUP) ||
      (BEEGFS_POLLNVAL != POLLNVAL) )
   {
      printk_fhgfs(KERN_WARNING, "Value of POLL... doesn't match");
      return fhgfs_false;
   }

   if(BEEGFS_MSG_DONTWAIT != MSG_DONTWAIT)
   {
      printk_fhgfs(KERN_WARNING, "Value of MSG_DONTWAIT: assumed: %d; actual: %d\n",
         (int)BEEGFS_MSG_DONTWAIT, (int)MSG_DONTWAIT);
      return fhgfs_false;
   }


   return fhgfs_true;
}

/**
 * Note: Better use SocketTk_getHostByAddrStr(), which can also check for errors on recent kernels.
 *
 * @return BEEGFS_INADDR_NONE if an error was detected (recent kernels only)
 */
fhgfs_in_addr SocketTk_in_aton(const char* hostAddr)
{
   fhgfs_in_addr retVal;

   // Note: retVal BEEGFS_INADDR_NONE will be set by getHostByAddrStr()

   SocketTk_getHostByAddrStr(hostAddr, &retVal);

   return retVal;
}

/**
 * Checks whether the poll() call was interrupted by a signal.
 *
 * Note: Some signals are ignored.
 * Note: Should be obsolete!! (because we're blocking signal by setting the sigmask now)
 *
 * @return fhgfs_true if a signal is pending that should interrupt the current poll (e.g. a SIGINT)
 */
fhgfs_bool __SocketTk_pollInterruptedBySignal(void)
{
   static const int interruptSignals[] = {SIGINT, SIGTERM, SIGKILL};
   static const size_t numInterruptSignals = sizeof(interruptSignals) / sizeof(int);

   size_t i;

   if(!signal_pending(current) )
      return fhgfs_false;

   for(i = 0; i < numInterruptSignals; i++)
   {
      if(sigismember(&current->pending.signal, interruptSignals[i] ) &&
         !sigismember(&current->blocked, interruptSignals[i] ) )
      {
         return fhgfs_true;
      }
   }

   return fhgfs_false;
}

/**
 * @return string is kalloced and needs to be kfreed
 */
char* SocketTk_ipaddrToStr(fhgfs_in_addr* ipaddress)
{
   unsigned char* charIP = (unsigned char*)ipaddress;

   const size_t ipStrLen = 16;
   char* ipStr = (char*)os_kmalloc(ipStrLen);

   sprintf(ipStr, "%u.%u.%u.%u", charIP[0], charIP[1], charIP[2], charIP[3]);

   return ipStr;
}

/**
 * @return string is kalloced and needs to be kfreed
 */
char* SocketTk_endpointAddrToString(fhgfs_in_addr* ipaddress, unsigned short port)
{
   char* endpointStr = (char*)os_kmalloc(SOCKETTK_ENDPOINTSTR_LEN);

   SocketTk_endpointAddrToStringNoAlloc(endpointStr, SOCKETTK_ENDPOINTSTR_LEN, ipaddress, port);

   return endpointStr;
}

/**
 * @param buf the buffer to which <IP>:<port> should be written.
 */
void SocketTk_endpointAddrToStringNoAlloc(char* buf, unsigned bufLen, fhgfs_in_addr* ipaddress,
   unsigned short port)
{
   unsigned char* charIP = (unsigned char*)ipaddress;

   int printRes = snprintf(buf, bufLen, "%u.%u.%u.%u:%u",
      charIP[0], charIP[1], charIP[2], charIP[3], port);

   if(unlikely( (unsigned)printRes >= bufLen) )
      buf[bufLen-1] = 0; // bufLen exceeded => zero-terminate result
}

