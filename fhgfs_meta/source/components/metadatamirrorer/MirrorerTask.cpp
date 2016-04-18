#include <common/app/log/LogContext.h>
#include <common/toolkit/serialization/Serialization.h>
#include <common/toolkit/MetaStorageTk.h>
#include <program/Program.h>
#include <storage/MetadataEx.h>
#include "MirrorerTask.h"

#include <attr/xattr.h>


unsigned MirrorerTask::serialize(char* buf)
{
   const char* logContext = "MirrorerTask::serialize";

   unsigned bufPos = 0;

   bufPos += Serialization::serializeInt(&buf[bufPos], taskType);


   // serialize taskType-dependent values

   switch(this->taskType)
   {
      case MirrorerTaskType_NEWDENTRY:
      {
         bufPos += Serialization::serializeUInt(&buf[bufPos], fileBufLen);
         bufPos += Serialization::serializeStrAlign4(&buf[bufPos],
            parentDirID.length(), parentDirID.c_str() );
         bufPos += Serialization::serializeStrAlign4(&buf[bufPos],
            entryName.length(), entryName.c_str() );
         bufPos += Serialization::serializeStrAlign4(&buf[bufPos],
            entryID.length(), entryID.c_str() );
         bufPos += Serialization::serializeUInt16(&buf[bufPos], mirroredFromNodeID);
         bufPos += Serialization::serializeBool(&buf[bufPos], hasEntryIDHardlink);

         memcpy(&buf[bufPos], fileBuf, fileBufLen);
         bufPos += fileBufLen;

         return bufPos;
      }

      case MirrorerTaskType_NEWINODE:
      {
         bufPos += Serialization::serializeUInt(&buf[bufPos], fileBufLen);
         bufPos += Serialization::serializeStrAlign4(&buf[bufPos],
            entryID.length(), entryID.c_str() );
         bufPos += Serialization::serializeUInt16(&buf[bufPos], mirroredFromNodeID);

         memcpy(&buf[bufPos], fileBuf, fileBufLen);
         bufPos += fileBufLen;

         return bufPos;
      }

      case MirrorerTaskType_REMOVEDENTRY:
      {
         bufPos += Serialization::serializeStrAlign4(&buf[bufPos],
            parentDirID.length(), parentDirID.c_str() );
         bufPos += Serialization::serializeStrAlign4(&buf[bufPos],
            entryName.length(), entryName.c_str() );
         bufPos += Serialization::serializeStrAlign4(&buf[bufPos],
            entryID.length(), entryID.c_str() );
         bufPos += Serialization::serializeUInt16(&buf[bufPos], mirroredFromNodeID);

         return bufPos;
      }

      case MirrorerTaskType_REMOVEINODE:
      {
         bufPos += Serialization::serializeStrAlign4(&buf[bufPos],
            entryID.length(), entryID.c_str() );
         bufPos += Serialization::serializeUInt16(&buf[bufPos], mirroredFromNodeID);
         bufPos += Serialization::serializeBool(&buf[bufPos], hasContentsDir);

         return bufPos;
      }

      case MirrorerTaskType_RENAMEINSAMEDIR:
      {
         bufPos += Serialization::serializeStrAlign4(&buf[bufPos],
            parentDirID.length(), parentDirID.c_str() );
         bufPos += Serialization::serializeStrAlign4(&buf[bufPos],
            entryName.length(), entryName.c_str() );
         bufPos += Serialization::serializeStrAlign4(&buf[bufPos],
            newEntryName.length(), newEntryName.c_str() );
         bufPos += Serialization::serializeUInt16(&buf[bufPos], mirroredFromNodeID);

         return bufPos;
      }

      case MirrorerTaskType_LINKINSAMEDIR:
      {
         bufPos += Serialization::serializeUInt(&buf[bufPos], fileBufLen);
         bufPos += Serialization::serializeStrAlign4(&buf[bufPos],
            parentDirID.length(), parentDirID.c_str() );
         bufPos += Serialization::serializeStrAlign4(&buf[bufPos],
            entryName.length(), entryName.c_str() );
         bufPos += Serialization::serializeStrAlign4(&buf[bufPos],
            newEntryName.length(), newEntryName.c_str() );
         bufPos += Serialization::serializeUInt16(&buf[bufPos], mirroredFromNodeID);

         memcpy(&buf[bufPos], fileBuf, fileBufLen);
         bufPos += fileBufLen;

         return bufPos;
      }

      default:
      {
         LogContext(logContext).logErr("Bug: Invalid taskType given: " +
            StringTk::intToStr(taskType) );
         LogContext(logContext).logBacktrace();

         return 0;
      }

   }

}

bool MirrorerTask::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
{
   const char* logContext = "MirrorerTask::deserialize";

   size_t bufPos = 0;

   { // taskType
      unsigned fieldBufLen;
      int taskTypeTmp;

      if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos,
         &taskTypeTmp, &fieldBufLen) )
         return false;

      taskType = (MirrorerTaskType)taskTypeTmp;
      bufPos += fieldBufLen;
   }

   // deserialize taskType-dependent values

   switch(this->taskType)
   {
      case MirrorerTaskType_NEWDENTRY:
      {
         { // recvFileBufLen
            unsigned fieldBufLen;

            if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
               &recvFileBufLen, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // parentDirID
            unsigned fieldBufLen = 0;

            if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
               &parentDirID, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // entryName
            unsigned fieldBufLen = 0;

            if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
               &entryName, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // entryID
            unsigned fieldBufLen = 0;

            if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
               &entryID, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // mirroredFromNodeID
            unsigned fieldBufLen;

            if(!Serialization::deserializeUInt16(&buf[bufPos], bufLen-bufPos,
               &mirroredFromNodeID, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // hasEntryIDHardlink
            unsigned fieldBufLen;

            if(!Serialization::deserializeBool(&buf[bufPos], bufLen-bufPos,
               &hasEntryIDHardlink, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // recvFileBuf
            recvFileBuf = &buf[bufPos];
            bufPos += recvFileBufLen;
         }

      } break;

      case MirrorerTaskType_NEWINODE:
      {
         { // recvFileBufLen
            unsigned fieldBufLen;

            if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
               &recvFileBufLen, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // entryID
            unsigned fieldBufLen = 0;

            if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
               &entryID, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // mirroredFromNodeID
            unsigned fieldBufLen;

            if(!Serialization::deserializeUInt16(&buf[bufPos], bufLen-bufPos,
               &mirroredFromNodeID, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // recvFileBuf
            recvFileBuf = &buf[bufPos];
            bufPos += recvFileBufLen;
         }

      } break;

      case MirrorerTaskType_REMOVEDENTRY:
      {
         { // parentDirID
            unsigned fieldBufLen = 0;

            if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
               &parentDirID, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // entryName
            unsigned fieldBufLen = 0;

            if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
               &entryName, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // entryID
            unsigned fieldBufLen = 0;

            if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
               &entryID, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // mirroredFromNodeID
            unsigned fieldBufLen;

            if(!Serialization::deserializeUInt16(&buf[bufPos], bufLen-bufPos,
               &mirroredFromNodeID, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }
      } break;

      case MirrorerTaskType_REMOVEINODE:
      {
         { // entryID
            unsigned fieldBufLen = 0;

            if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
               &entryID, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // mirroredFromNodeID
            unsigned fieldBufLen;

            if(!Serialization::deserializeUInt16(&buf[bufPos], bufLen-bufPos,
               &mirroredFromNodeID, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // hasContentsDir
            unsigned fieldBufLen;

            if(!Serialization::deserializeBool(&buf[bufPos], bufLen-bufPos,
               &hasContentsDir, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }
      } break;

      case MirrorerTaskType_RENAMEINSAMEDIR:
      {
         { // parentDirID
            unsigned fieldBufLen = 0;

            if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
               &parentDirID, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // entryName
            unsigned fieldBufLen = 0;

            if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
               &entryName, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // entryID
            unsigned fieldBufLen = 0;

            if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
               &newEntryName, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // mirroredFromNodeID
            unsigned fieldBufLen;

            if(!Serialization::deserializeUInt16(&buf[bufPos], bufLen-bufPos,
               &mirroredFromNodeID, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }
      } break;

      case MirrorerTaskType_LINKINSAMEDIR:
      {
         { // recvFileBufLen
            unsigned fieldBufLen;

            if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
               &recvFileBufLen, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // parentDirID
            unsigned fieldBufLen = 0;

            if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
               &parentDirID, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // entryName
            unsigned fieldBufLen = 0;

            if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
               &entryName, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // entryID
            unsigned fieldBufLen = 0;

            if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
               &newEntryName, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // mirroredFromNodeID
            unsigned fieldBufLen;

            if(!Serialization::deserializeUInt16(&buf[bufPos], bufLen-bufPos,
               &mirroredFromNodeID, &fieldBufLen) )
               return false;

            bufPos += fieldBufLen;
         }

         { // recvFileBuf
            recvFileBuf = &buf[bufPos];
            bufPos += recvFileBufLen;
         }

      } break;

      default:
      {
         LogContext(logContext).logErr("Bug: Invalid taskType given: " +
            StringTk::intToStr(taskType) );
         LogContext(logContext).logBacktrace();

         return false;
      } break;

   }


   *outLen = bufPos;

   return true;
}

/**
 * @return required serialization buffer length; 0 if this task requires loading of a metadata file
 * and the file was not found (or could not be read).
 */
unsigned MirrorerTask::calcSerialLen()
{
   const char* logContext = "MirrorerTask::serialLen";

   // compute length of this particular task

   switch(this->taskType)
   {
      case MirrorerTaskType_NEWDENTRY:
      {
         return Serialization::serialLenInt() + // taskType
            Serialization::serialLenUInt() + // length of attached fileBuf
            Serialization::serialLenStrAlign4(parentDirID.length() ) +
            Serialization::serialLenStrAlign4(entryName.length() ) +
            Serialization::serialLenStrAlign4(entryID.length() ) +
            Serialization::serialLenUInt16() + // mirroredFromNodeID
            Serialization::serialLenBool() + // hasEntryIDHardlink
            fileBufLen; // filebuf
      }

      case MirrorerTaskType_NEWINODE:
      {
         return Serialization::serialLenInt() + // taskType
            Serialization::serialLenUInt() + // length of attached fileBuf
            Serialization::serialLenStrAlign4(entryID.length() ) +
            Serialization::serialLenUInt16() + // mirroredFromNodeID
            fileBufLen; // fileBuf
      }

      case MirrorerTaskType_REMOVEDENTRY:
      {
         return Serialization::serialLenInt() + // taskType
            Serialization::serialLenStrAlign4(parentDirID.length() ) +
            Serialization::serialLenStrAlign4(entryName.length() ) +
            Serialization::serialLenStrAlign4(entryID.length() ) +
            Serialization::serialLenUInt16(); // mirroredFromNodeID
      }

      case MirrorerTaskType_REMOVEINODE:
      {
         return Serialization::serialLenInt() + // taskType
            Serialization::serialLenStrAlign4(entryID.length() ) +
            Serialization::serialLenUInt16() + // mirroredFromNodeID
            Serialization::serialLenBool(); // hasContentsDir
      }

      case MirrorerTaskType_RENAMEINSAMEDIR:
      {
         return Serialization::serialLenInt() + // taskType
            Serialization::serialLenStrAlign4(parentDirID.length() ) +
            Serialization::serialLenStrAlign4(entryName.length() ) +
            Serialization::serialLenStrAlign4(newEntryName.length() ) +
            Serialization::serialLenUInt16(); // mirroredFromNodeID
      }

      case MirrorerTaskType_LINKINSAMEDIR:
      {
         return Serialization::serialLenInt()                           +  // taskType
            Serialization::serialLenUInt()                              +  // fileBufLen
            Serialization::serialLenStrAlign4(parentDirID.length() )    +
            Serialization::serialLenStrAlign4(entryName.length() )      +
            Serialization::serialLenStrAlign4(newEntryName.length() )   +
            Serialization::serialLenUInt16()                            + // mirroredFromNodeID
            fileBufLen;                                                   // filebuf
      }

      default:
      {
         LogContext(logContext).logErr("Bug: Invalid taskType given: " +
            StringTk::intToStr(taskType) );
         LogContext(logContext).logBacktrace();

         return 0;
      }

   }
}

bool MirrorerTask::loadFileBufIfNeeded()
{
   if(fileBuf)
      return true; // already loaded

   App* app = Program::getApp();
   bool useXAttrs = app->getConfig()->getStoreUseExtendedAttribs();

   switch(this->taskType)
   {
      case MirrorerTaskType_NEWDENTRY:
      {

         // path to parent's contents dir
         std::string parentPath = MetaStorageTk::getMetaDirEntryPath(
            app->getDentriesPath()->getPathAsStrConst(), parentDirID);

         std::string dentryPath;

         // (prefer the entryID file if we have it - because names change, IDs don't)
         if(!entryID.empty() )
            dentryPath = MetaStorageTk::getMetaDirEntryIDPath(parentPath) + "/" + entryID;
         else
            dentryPath = parentPath + "/" + entryName;

         if(useXAttrs)
            return loadFileBufFromXattr(dentryPath, META_XATTR_LINK_NAME);
         else
            return loadFileBufFromContents(dentryPath);
      }

      case MirrorerTaskType_NEWINODE:
      {
         std::string path = MetaStorageTk::getMetaInodePath(
            app->getInodesPath()->getPathAsStrConst(), entryID);

         if(useXAttrs)
            return loadFileBufFromXattr(path, META_XATTR_LINK_NAME);
         else
            return loadFileBufFromContents(path);
      }

      default:
      { // no file buf loading required
         return true;
      }

   } // end of switch statement
}


/**
 * Load fileBuf from file xattr.
 *
 * Note: Intended to be only be called via loadFileBufIfNeeded()
 */
bool MirrorerTask::loadFileBufFromXattr(std::string path, const char* xattrName)
{
   const char* logContext = "MirrorerTask (load from xattr file)";

   this->fileBuf = (char*)malloc(META_SERBUF_SIZE);

   ssize_t getRes = getxattr(path.c_str(), META_XATTR_DIR_NAME, fileBuf, META_SERBUF_SIZE);
   if(getRes > 0)
   { // we got something
      fileBufLen = getRes;
      return true;
   }

   // loading failed

   if( (getRes == -1) && (errno == ENOENT) )
   { // file not exists
      LOG_DEBUG(logContext, Log_DEBUG, "Metadata file not exists: " + path + ". "
         "SysErr: " + System::getErrString() );
   }
   else
   { // unhandled error
      LogContext(logContext).log(Log_DEBUG, "Unable to load xattr metadata file: " + path + ". "
         "SysErr: " + System::getErrString() );
   }

   free(fileBuf);
   fileBuf = NULL;

   return false;
}

/**
 * Load fileBuf from normal file contents (i.e. no xattr).
 *
 * Note: Intended to be only be called via loadFileBufIfNeeded()
 */
bool MirrorerTask::loadFileBufFromContents(std::string path)
{
   const char* logContext = "MirrorerTask (load from file contents)";

   int openFlags = O_NOATIME | O_RDONLY;

   int fd = open(path.c_str(), openFlags, 0);
   if(fd == -1)
   { // open failed
      if(errno == ENOENT)
         LOG_DEBUG(logContext, Log_DEBUG, "Metadata file not exists: " + path + ". "
            "SysErr: " + System::getErrString() );
      else
         LogContext(logContext).logErr("Unable to open metadata file: " + path + ". "
            "SysErr: " + System::getErrString() );

      return false;
   }

   this->fileBuf = (char*)malloc(META_SERBUF_SIZE);

   int readRes = read(fd, fileBuf, META_SERBUF_SIZE);
   if(readRes > 0)
   { // we got something
      fileBufLen = readRes;
      close(fd);
      return true;
   }

   // loading failed

   LogContext(logContext).log(Log_DEBUG, "Unable to read metadata file: " + path + ". " +
      "SysErr: " + System::getErrString() );

   free(fileBuf);
   fileBuf = NULL;
   close(fd);

   return false;
}

/**
 * Called by the receiving mirror node to execute the task.
 */
FhgfsOpsErr MirrorerTask::executeTask()
{
   const char* logContext = "MirrorerTask (execute)";

   App* app = Program::getApp();

   switch(this->taskType)
   {
      case MirrorerTaskType_NEWDENTRY:
      {
         // path to parent's contents dir
         std::string parentPath = MetaStorageTk::getMetaMirroredDirEntryPath(
            app->getMetaPath(), parentDirID, mirroredFromNodeID);

         if(entryID.empty() )
         { // no entryID given => just save dentry name file
            std::string dentryNamePath = parentPath + "/" + entryName;

            return saveRecvFileBuf(dentryNamePath);
         }
         else
         { // entryID given => save buffer to it
            std::string dentryIDPath =
               MetaStorageTk::getMetaDirEntryIDPath(parentPath) + "/" + entryID;

            FhgfsOpsErr saveRes = saveRecvFileBuf(dentryIDPath);

            if(unlikely(saveRes != FhgfsOpsErr_SUCCESS) )
               return saveRes;

            if(hasEntryIDHardlink)
            { // we shall also link dentry ID file to name file
               std::string dentryNamePath = parentPath + "/" + entryName;

               return linkFile(dentryIDPath, dentryNamePath);
            }

            return saveRes;
         }

      } break;

      case MirrorerTaskType_NEWINODE:
      {
         /* note: contents dir will be created on demand if a dentry file is stored inside it,
            so we do not need to create it here. */

         std::string path = MetaStorageTk::getMetaMirroredInodePath(
            app->getMetaPath(), entryID, mirroredFromNodeID);

         return saveRecvFileBuf(path);
      } break;

      case MirrorerTaskType_REMOVEDENTRY:
      {
         FhgfsOpsErr unlinkNameRes = FhgfsOpsErr_SUCCESS;
         FhgfsOpsErr unlinkIDRes = FhgfsOpsErr_SUCCESS;

         std::string parentPath = MetaStorageTk::getMetaMirroredDirEntryPath(
            app->getMetaPath(), parentDirID, mirroredFromNodeID);

         if(!entryName.empty() )
         {
            std::string dentryNamePath = parentPath + "/" + entryName;

            unlinkNameRes = unlinkFile(dentryNamePath);
         }

         if(!entryID.empty() )
         {
            std::string dentryIDPath =
               MetaStorageTk::getMetaDirEntryIDPath(parentPath) + "/" + entryID;

            unlinkIDRes = unlinkFile(dentryIDPath);
         }

         return (unlinkNameRes != FhgfsOpsErr_SUCCESS) ? unlinkNameRes : unlinkIDRes;

      } break;

      case MirrorerTaskType_REMOVEINODE:
      {
         std::string path = MetaStorageTk::getMetaMirroredInodePath(
            app->getMetaPath(), entryID, mirroredFromNodeID);

         FhgfsOpsErr totalRes = FhgfsOpsErr_SUCCESS;

         FhgfsOpsErr unlinkFileRes = unlinkFile(path);
         if(unlikely(unlinkFileRes != FhgfsOpsErr_SUCCESS) )
            totalRes = unlinkFileRes;

         if(hasContentsDir)
         { // also remove contents dir
            std::string contentsPath = MetaStorageTk::getMetaMirroredDirEntryPath(
               app->getMetaPath(), entryID, mirroredFromNodeID);

            FhgfsOpsErr rmNamesRes = removeContentsDir(contentsPath);
            if(unlikely(rmNamesRes != FhgfsOpsErr_SUCCESS) )
               if(totalRes == FhgfsOpsErr_SUCCESS)
                  totalRes = rmNamesRes; // set error result if unset
         }

         return totalRes;

      } break;

      case MirrorerTaskType_RENAMEINSAMEDIR:
      {
         std::string parentPath = MetaStorageTk::getMetaMirroredDirEntryPath(
            app->getMetaPath(), parentDirID, mirroredFromNodeID);
         std::string fromPath = parentPath + "/" + entryName;
         std::string toPath = parentPath + "/" + newEntryName;

         return renameFile(fromPath, toPath);

      } break;

      case MirrorerTaskType_LINKINSAMEDIR:
      {
         std::string parentPath = MetaStorageTk::getMetaMirroredDirEntryPath(
            app->getMetaPath(), parentDirID, mirroredFromNodeID);
         std::string fromPath = parentPath + "/" + entryName;
         std::string toPath = parentPath + "/" + newEntryName;

         // update inode attributes first (inode inlined into the dentry)
         FhgfsOpsErr saveRes = saveRecvFileBuf(fromPath);
         if(unlikely(saveRes != FhgfsOpsErr_SUCCESS) )
            return saveRes;

         return linkFile(fromPath, toPath);

      } break;

      default:
      {
         LogContext(logContext).logErr("Bug: Invalid taskType given: " +
            StringTk::intToStr(taskType) );
         LogContext(logContext).logBacktrace();

         return FhgfsOpsErr_INTERNAL;
      } break;

   } // end of switch statement
}

/**
 * Save recvFileBuf to file. Works for xattr and normal file contents.
 *
 * Note: Intended to be only be called via saveRecvFileBufIfNeeded()
 *
 * @return false on error
 */
FhgfsOpsErr MirrorerTask::saveRecvFileBuf(std::string path)
{
   const char* logContext = "MirrorerTask (save file)";

   App* app = Program::getApp();
   bool useXAttrs = app->getConfig()->getStoreUseExtendedAttribs();

   int closeRes;

   // create file

   int openFlags = O_CREAT|O_WRONLY;
   mode_t openMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

   int fd = open(path.c_str(), openFlags, openMode);
   if(fd == -1)
   {  // hash dir just doesn't exist yet or real error?

      if(likely(errno == ENOENT) )
      { // dir just didn't exist yet => create it and open again
         Path filePath(path);

         bool createPathRes = StorageTk::createPathOnDisk(filePath, true);
         if(!createPathRes)
         {
            LogContext(logContext).logErr(
               "Unable to create path for metadata mirror file: " + path + ". "
               "SysErr: " + System::getErrString() );

            return FhgfsOpsErr_INTERNAL;
         }

         // dir created => try file open/create again...

         fd = open(path.c_str(), openFlags, openMode);
      }

      if(unlikely(fd == -1) )
      { // this is a real error
         LogContext(logContext).logErr(
            "Unable to create metadata mirror file: " + path + ". "
            "SysErr: " + System::getErrString() );

         return FhgfsOpsErr_INTERNAL;
      }
   }

   // write data to file

   if(useXAttrs)
   { // extended attribute
      int setRes = fsetxattr(fd, META_XATTR_LINK_NAME, recvFileBuf, recvFileBufLen, 0);

      if(unlikely(setRes == -1) )
      { // error
         if(errno == ENOTSUP)
            LogContext(logContext).logErr(
               "Unable to store xattr metadata mirror file: " + path + ". "
               "Did you enable extended attributes (user_xattr) on the underlying file system?");
         else
            LogContext(logContext).logErr(
               "Unable to store xattr metadata mirror file: " + path + ". "
               "SysErr: " + System::getErrString() );

         goto error_closefile;
      }
   }
   else
   { // normal file content
      ssize_t writeRes = write(fd, recvFileBuf, recvFileBufLen);

      if(unlikely(writeRes != (ssize_t)recvFileBufLen) )
      {
         LogContext(logContext).logErr("Unable to store metadata mirror file: " + path + ". "
            "SysErr: " + System::getErrString() );

         goto error_closefile;
      }
   }

   closeRes = close(fd);
   if(unlikely(closeRes == -1) )
   { // close error
      LogContext(logContext).logErr("Closing metadata mirror file failed: " + path + ". "
         "SysErr: " + System::getErrString() );
   }

   LOG_DEBUG(logContext, Log_SPAM, "Metadata mirror file stored: " + path);

   return FhgfsOpsErr_SUCCESS;


   // error compensation
error_closefile:
   close(fd);

   return FhgfsOpsErr_INTERNAL;
}

/**
 * Create a hard link.
 */
FhgfsOpsErr MirrorerTask::linkFile(std::string fromPath, std::string toPath)
{
   const char* logContext = "MirrorerTask (hardlink saved file)";

   int linkRes = link(fromPath.c_str(), toPath.c_str() );
   if(unlikely(linkRes == -1) )
   {
      LogContext(logContext).logErr("Unable to create metadata file hardlink. "
         "From: " + fromPath + "; "
         "To: " + toPath + "; "
         "SysErr: " + System::getErrString() );

      return FhgfsOpsErr_INTERNAL;
   }

   LOG_DEBUG(logContext, Log_SPAM, "Metadata hardlink created. "
      "From: " + fromPath + "; "
      "To: " + toPath);

   return FhgfsOpsErr_SUCCESS;
}

/**
 * Unlink a file.
 */
FhgfsOpsErr MirrorerTask::unlinkFile(std::string path)
{
   const char* logContext = "MirrorerTask (unlink file)";

   int unlinkRes = unlink(path.c_str() );
   if(unlikely(unlinkRes == -1) )
   {
      // (we consider removal of non-existing file as success, because it shouldn't be a problem)
      if(errno != ENOENT)
      {
         LogContext(logContext).logErr("Unable to unlink metadata file: " + path + "; "
            "SysErr: " + System::getErrString() );

         return FhgfsOpsErr_INTERNAL;
      }
   }

   LOG_DEBUG(logContext, Log_SPAM, "Metadata file unlinked: " + path);

   return FhgfsOpsErr_SUCCESS;
}

/**
 * Remove contents directory of a dir inode.
 *
 * Note: This also rmdir()s the fsids subdir first.
 */
FhgfsOpsErr MirrorerTask::removeContentsDir(std::string path)
{
   const char* logContext = "MirrorerTask (remove dir)";

   std::string idPath = MetaStorageTk::getMetaDirEntryIDPath(path);

   int rmIDRes = rmdir(idPath.c_str() );
   if(unlikely(rmIDRes == -1) )
   {
      /* note: if the dir we want to remove doesn't exist, it's not an error, because we create dirs
         only on demand, so it's normal that a dir might not exist. */
      if(errno != ENOENT)
      {
         LogContext(logContext).logErr("Unable to remove metadata dir: " + idPath + "; "
            "SysErr: " + System::getErrString() );

         return FhgfsOpsErr_INTERNAL;
      }
   }

   int rmRes = rmdir(path.c_str() );
   if(unlikely(rmRes == -1) )
   {
      /* note: if the dir we want to remove doesn't exist, it's not an error, because we create dirs
         only on demand, so it's normal that a dir might not exist. */
      if(errno != ENOENT)
      {
         LogContext(logContext).logErr("Unable to remove metadata dir: " + idPath + "; "
            "SysErr: " + System::getErrString() );

         return FhgfsOpsErr_INTERNAL;
      }
   }

   LOG_DEBUG(logContext, Log_SPAM, "Metadata dir removed: " + path);

   return FhgfsOpsErr_SUCCESS;
}

/**
 * Rename(/move) a file.
 */
FhgfsOpsErr MirrorerTask::renameFile(std::string fromPath, std::string toPath)
{
   const char* logContext = "MirrorerTask (rename saved file)";

   int renameRes = rename(fromPath.c_str(), toPath.c_str() );
   if(unlikely(renameRes == -1) )
   {
      LogContext(logContext).logErr("Unable to rename metadata file. "
         "From: " + fromPath + "; "
         "To: " + toPath + "; "
         "SysErr: " + System::getErrString() );

      return FhgfsOpsErr_INTERNAL;
   }

   LOG_DEBUG(logContext, Log_SPAM, "Metadata file renamed. "
      "From: " + fromPath + "; "
      "To: " + toPath);

   return FhgfsOpsErr_SUCCESS;
}

