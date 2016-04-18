// control messages
#include <common/net/message/control/AuthenticateChannelMsgEx.h>
#include <common/net/message/control/GenericResponseMsg.h>
#include "control/SetChannelDirectMsgEx.h"
// helperd messages
#include "helperd/GetHostByNameMsgEx.h"
#include <common/net/message/helperd/GetHostByNameRespMsg.h>
#include "helperd/LogMsgEx.h"
#include <common/net/message/helperd/LogRespMsg.h>


#include <common/net/message/SimpleMsg.h>
#include "NetMessageFactory.h"

/**
 * @return NetMessage that must be deleted by the caller
 * (msg->msgType is NETMSGTYPE_Invalid on error)
 */
NetMessage* NetMessageFactory::createFromMsgType(unsigned short msgType)
{
   NetMessage* msg;
   
   switch(msgType)
   {
      // control messages
      case NETMSGTYPE_GenericResponse: { msg = new GenericResponseMsg(); } break;
      case NETMSGTYPE_SetChannelDirect: { msg = new SetChannelDirectMsgEx(); } break;
      case NETMSGTYPE_AuthenticateChannel: { msg = new AuthenticateChannelMsgEx(); } break;
      // helperd messages
      case NETMSGTYPE_GetHostByName: { msg = new GetHostByNameMsgEx(); } break;
      case NETMSGTYPE_GetHostByNameResp: { msg = new GetHostByNameRespMsg(); } break;
      case NETMSGTYPE_Log: { msg = new LogMsgEx(); } break;
      case NETMSGTYPE_LogResp: { msg = new LogRespMsg(); } break;
      
      default:
      {
         msg = new SimpleMsg(NETMSGTYPE_Invalid);
      } break;
   }
   
   return msg;
}

