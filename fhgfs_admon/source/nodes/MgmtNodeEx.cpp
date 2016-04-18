#include "MgmtNodeEx.h"
#include <program/Program.h>

#define MGMTDNODEDATA_LOGFILEPATH "/var/log/beegfs-mgmtd.log"


MgmtNodeEx::MgmtNodeEx(std::string nodeID, uint16_t nodeNumID, unsigned short portUDP,
   unsigned short portTCP, NicAddressList& nicList) :
   Node(nodeID, nodeNumID, portUDP, portTCP, nicList)
{
   initialize();
}

void MgmtNodeEx::initialize()
{
	   data.isResponding = true;

	   generalInfo.cpuCount = 0;
	   generalInfo.cpuName = "";
	   generalInfo.cpuSpeed = 0;
	   generalInfo.memFree = 0;
	   generalInfo.memTotal = 0;
	   generalInfo.logFilePath = MGMTDNODEDATA_LOGFILEPATH;
}

