#include "NetworkInterfaceCard.h"

#include <common/toolkit/ListTk.h>
#include <common/toolkit/StringTk.h>
#include <common/app/log/LogContext.h>
#include <common/system/System.h>
#include "Socket.h"
#include "RDMASocket.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <net/if_arp.h>
#include <arpa/inet.h>

#define IFRSIZE(numIfcReqs)      ((int)(numIfcReqs * sizeof(struct ifreq) ) )


/**
 * @return true if any usable standard interface was found
 */
bool NetworkInterfaceCard::findAll(StringList* allowedInterfacesList, bool useSDP, bool useRDMA,
   NicAddressList* outList)
{
   bool retVal = false;
   
   // find standard TCP/IP interfaces
   if(findAllBySocketDomain(PF_INET, NICADDRTYPE_STANDARD, allowedInterfacesList, outList) )
      retVal = true;
   
   // find SDP interfaces
   if(useSDP)
   {
      findAllBySocketDomain(PF_SDP, NICADDRTYPE_SDP, allowedInterfacesList, outList);
   }
   
   // find RDMA interfaces (based on TCP/IP interfaces query results)
   if(useRDMA && RDMASocket::rdmaDevicesExist() )
   {
      NicAddressList tcpInterfaces;
      
      findAllBySocketDomain(PF_INET, NICADDRTYPE_STANDARD, allowedInterfacesList, &tcpInterfaces);

      filterInterfacesForRDMA(&tcpInterfaces, outList);
   }
   
   return retVal;
}

bool NetworkInterfaceCard::findAllBySocketDomain(int domain, NicAddrType nicType,
   StringList* allowedInterfacesList, NicAddressList* outList)
{
   int sock = socket(domain, SOCK_STREAM, 0);
   if(sock == -1)
      return false;


   int numIfcReqs = 1; // number of interfaces that can be stored in the current buffer (can grow)
   
   struct ifconf ifc;
   ifc.ifc_len = IFRSIZE(numIfcReqs);
   ifc.ifc_req = NULL;

   // enlarge the buffer to store all existing interfaces
   do
   {
      numIfcReqs++; // grow bufferspace to one more interface
      SAFE_FREE(ifc.ifc_req); // free previous allocation (if any)
      
      // alloc buffer for interfaces query
      ifc.ifc_req = (ifreq*)malloc(IFRSIZE(numIfcReqs) );
      if(!ifc.ifc_req)
      {
         LogContext("NIC query").logErr("Out of memory");
         close(sock);

         return false;
      }
      
      ifc.ifc_len = IFRSIZE(numIfcReqs);

      if(ioctl(sock, SIOCGIFCONF, &ifc) )
      {
         LogContext("NIC query").logErr(
            std::string("ioctl SIOCGIFCONF failed: ") + System::getErrString() );
         free(ifc.ifc_req);
         close(sock);

         return false;
      }
      
      /* ifc.ifc_len was updated by ioctl, so if IRSIZE<=ifc.ifc_len then there might be more
         interfaces, so loop again with larger buffer... */

   } while (IFRSIZE(numIfcReqs) <= ifc.ifc_len);


   // foreach interface

   struct ifreq* ifr = ifc.ifc_req; // pointer to current interface
   
   for( ; (char*) ifr < (char*) ifc.ifc_req + ifc.ifc_len; ifr++)
   {
      NicAddress nicAddr;
      
      if(fillNicAddress(sock, nicType, ifr, &nicAddr) )
      {
         ssize_t metricByListPos = 0;
         
         if(allowedInterfacesList->size() &&
            !ListTk::listContains(nicAddr.name, allowedInterfacesList, &metricByListPos) )
            continue; // not in the list of allowed interfaces
         
         nicAddr.metric = (int)metricByListPos;
         outList->push_back(nicAddr);
      }
   }


   free(ifc.ifc_req);
   close(sock);
   
   return true;
}

/**
 * Checks a list of TCP/IP interfaces for RDMA-capable interfaces.
 */
void NetworkInterfaceCard::filterInterfacesForRDMA(NicAddressList* list, NicAddressList* outList)
{
   // Note: This works by binding an RDMASocket to each IP of the passed list.
   
   for(NicAddressListIter iter = list->begin(); iter != list->end(); iter++)
   {
      try
      {
         RDMASocket rdmaSock;

         rdmaSock.bindToAddr(iter->ipAddr.s_addr, 0);
         
         // interface is RDMA-capable => append to outList
         
         NicAddress nicAddr = *iter;
         nicAddr.nicType = NICADDRTYPE_RDMA;
         
         outList->push_back(nicAddr);
      }
      catch(SocketException& e)
      {
         // interface is not RDMA-capable in this case
      }
   }
}

/**
 * This is not needed in the actual app.
 * Nevertheless, it's for some reason part of some tests.
 */
bool NetworkInterfaceCard::findByName(const char* interfaceName, NicAddress* outAddr)
{
   int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_IP);
   if(sock == -1)
      return false;

   //struct ifreq* ifr;
   struct ifreq ifrr;
   //struct sockaddr_in sa;

   //ifr = &ifrr;
   ifrr.ifr_addr.sa_family = AF_INET;
   StringTk::strncpyTerminated(ifrr.ifr_name, interfaceName, sizeof(ifrr.ifr_name) );

   int fillRes = false;
   if(!ioctl(sock, SIOCGIFADDR, &ifrr) )
      fillRes = fillNicAddress(sock, NICADDRTYPE_STANDARD, &ifrr, outAddr);

   close(sock);

   return fillRes;
}

/**
 * @param ifr interface name and IP must be set in ifr when this method is called
 */
bool NetworkInterfaceCard::fillNicAddress(int sock, NicAddrType nicType, struct ifreq* ifr,
   NicAddress* outAddr)
{
   // note: struct ifreq contains a union for everything except the name. hence, new ioctl()
   //    calls overwrite the old data.
   
   // IP address
   // note: must be done at the beginning because following ioctl-calls will overwrite the data
   outAddr->ipAddr = ( (struct sockaddr_in *)&ifr->ifr_addr)->sin_addr;

   // name
   // note: must be done at the beginning because following ioctl-calls will overwrite the data
   strcpy(outAddr->name, ifr->ifr_name);

   
   // retrieve broadcast address
   if(ioctl(sock, SIOCGIFBRDADDR, ifr) )
      return false;
   
   outAddr->broadcastAddr = ( (struct sockaddr_in *)&ifr->ifr_broadaddr)->sin_addr;

   
   // retrieve bandwidth
   if(ioctl(sock, SIOCGIFINDEX, ifr) )
      return false;
   
   outAddr->bandwidth = ifr->ifr_ifru.ifru_ivalue;
   
   
   // retrieve flags
   if(ioctl(sock, SIOCGIFFLAGS, ifr) )
      return false;

   if(ifr->ifr_flags & IFF_LOOPBACK)
      return false; // loopback interface => skip


   
   // hardware type and address
   if(ioctl(sock, SIOCGIFHWADDR, ifr) )
      return false;
   else
   {
      // select which hardware types to process
      // (see /usr/include/linux/if_arp.h for the whole the list)
      switch(ifr->ifr_hwaddr.sa_family)
      {
         case ARPHRD_LOOPBACK: return false;
         
         default:
         {
            // make sure we allow SDP for IB only (because an SDP socket domain is valid for other
            // NIC types as well, but cannot connect then, of course
            if( (nicType == NICADDRTYPE_SDP) && (ifr->ifr_hwaddr.sa_family != ARPHRD_INFINIBAND) )
               return false;
         } break;
         
         // explicit types no longer required, because we identify the hardware by the socket type
         // now (just the loopback should be avoided)
         /*
         case ARPHRD_INFINIBAND:
         {
            outAddr->nicType = NICADDRTYPE_SDP;
            //return false;
         } break;

         case ARPHRD_NETROM:
         case ARPHRD_ETHER:
         case ARPHRD_PPP:
         case ARPHRD_EETHER:
         case ARPHRD_IEEE802:
         {
            outAddr->nicType = NICADDRTYPE_STANDARD;
         } break;

         default:
         { // ignore everything else
            return false;
         } break;
         */
      }
      
      // copy nicType
      outAddr->nicType = nicType;

      // copy hardware address
      memcpy(&outAddr->hwAddr, &ifr->ifr_addr.sa_data, IFHWADDRLEN);
   }


   // metric
   if(ioctl(sock, SIOCGIFMETRIC, ifr) )
      return false;
   else
   {
      outAddr->metric = ifr->ifr_metric;
   }
   
   
   return true;
}

/**
 * @return static string (not alloc'ed, so don't free it).
 */
const char* NetworkInterfaceCard::nicTypeToString(NicAddrType nicType)
{
   switch(nicType)
   {
      case NICADDRTYPE_RDMA: return "RDMA";
      case NICADDRTYPE_STANDARD: return "TCP";
      case NICADDRTYPE_SDP: return "SDP";

      default: return "<unknown>";
   }
}

std::string NetworkInterfaceCard::nicAddrToString(NicAddress* nicAddr)
{
   std::string resultStr;
   
   resultStr += nicAddr->name;
   resultStr += "[";
   
   resultStr += std::string("ip addr: ") + Socket::ipaddrToStr(&nicAddr->ipAddr) + "; ";
   resultStr += std::string("hw addr: ") + hwAddrToString(nicAddr->hwAddr) + "; ";
   resultStr += std::string("metric: ") + StringTk::intToStr(nicAddr->metric) + "; ";
   resultStr += std::string("bandwidth: ") + StringTk::intToStr(nicAddr->bandwidth) + "; ";
   resultStr += std::string("type: ") + nicTypeToString(nicAddr->nicType);
   
   resultStr += "]";
   
   return resultStr;
}

std::string NetworkInterfaceCard::nicAddrToStringLight(NicAddress* nicAddr)
{
   std::string resultStr;
   
   resultStr += nicAddr->name;
   resultStr += "[";
   
   resultStr += std::string("ip addr: ") + Socket::ipaddrToStr(&nicAddr->ipAddr) + "; ";
   resultStr += std::string("type: ") + nicTypeToString(nicAddr->nicType);
   
   resultStr += "]";
   
   return resultStr;
}


std::string NetworkInterfaceCard::hwAddrToString(char* hwAddr)
{
   char addrPartStr[5];
   std::string hwAddrStr;

   { // first loop walk (without the dot)    
      unsigned addrPart = (unsigned char)(hwAddr[0]);
      sprintf(addrPartStr, "%2.2x", addrPart);
      
      hwAddrStr += addrPartStr;
   }

   for(int i=1; i < IFHWADDRLEN; i++)
   {
      unsigned addrPart = (unsigned char)(hwAddr[i]);
      sprintf(addrPartStr, ".%2.2x", addrPart);
      
      hwAddrStr += addrPartStr;
   }
   
   return(hwAddrStr);
}

bool NetworkInterfaceCard::supportsSDP(NicAddressList* nicList)
{
   for(NicAddressListIter iter = nicList->begin(); iter != nicList->end(); iter++)
   {
      if(iter->nicType == NICADDRTYPE_SDP)
         return true;
   }
   
   return false;
}

bool NetworkInterfaceCard::supportsRDMA(NicAddressList* nicList)
{
   for(NicAddressListIter iter = nicList->begin(); iter != nicList->end(); iter++)
   {
      if(iter->nicType == NICADDRTYPE_RDMA)
         return true;
   }
   
   return false;
}

void NetworkInterfaceCard::supportedCapabilities(NicAddressList* nicList,
         NicListCapabilities* outCapabilities)
{
   outCapabilities->supportsSDP = supportsSDP(nicList);
   outCapabilities->supportsRDMA = supportsRDMA(nicList);
}





