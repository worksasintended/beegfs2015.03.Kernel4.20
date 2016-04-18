// O B S O L E T E
// O B S O L E T E
// O B S O L E T E

//#include <program/Program.h>
//#include <toolkit/StorageTk.h>
//#include <common/net/message/storage/creating/MkLocalFileRespMsg.h>
//#include "MkLocalFileMsgEx.h"
//
//
//bool MkLocalFileMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
//   char* respBuf, size_t bufLen, HighResolutionStats* stats)
//
//{
//   LogContext log("MkLocalFileMsg incoming");
//
//   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername(); 
//   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a MkLocalFileMsg from: ") + peer);
//
//   Path path;
//   parsePath(&path);
//   
//   SyncedDiskAccessPath* storagePath = Program::getApp()->getStoragePath();
//   Path fileStoragePath(*storagePath, path);
//   
//   // create path directories (not needed, because files are not in subdirs)
//   //storagePath->createSubPathOnDisk(path, true);
//
//   // create file
//   int openFlags = O_CREAT|O_RDWR|O_TRUNC|O_LARGEFILE;
//   mode_t openMode = S_IRWXU|S_IRWXG|S_IRWXO;
//
//   int fd = open(fileStoragePath.getPathAsStr().c_str(), openFlags, openMode);
//   if(fd == -1)
//   { // error
//      log.logErr("Unable to create file: " + fileStoragePath.getPathAsStr() + ". " +
//         "SysErr: " + System::getErrString() );
//   }
//   else
//   { // success
//      close(fd);
//      log.log(3, "File created: " + path.getPathAsStr() );
//   }
//
//   MkLocalFileRespMsg respMsg(fd == -1);
//   respMsg.serialize(respBuf, bufLen);
//   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
//      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
//
//   return true;      
//}


