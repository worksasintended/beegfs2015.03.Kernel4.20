#ifndef DISKLIST_H_
#define DISKLIST_H_

#include <common/threading/Mutex.h>
#include <common/toolkit/serialization/Serialization.h>

#include <cerrno>
#include <stdexcept>
#include <string>
#include <vector>

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <boost/scoped_array.hpp>

struct DiskListDoesNotExist : public std::runtime_error
{
   DiskListDoesNotExist(const std::string& file)
      : runtime_error(file)
   {}
};

template<typename Data>
class DiskList;

template<typename Data>
class DiskListCursor
{
   friend class DiskList<Data>;

   public:
      bool step()
      {
         uint64_t itemLen;

         {
            ssize_t readRes = ::pread(fd, &itemLen, sizeof(itemLen), offset);

            if (readRes == 0)
               return false;

            if (readRes < (ssize_t) sizeof(itemLen))
               throw std::runtime_error("could not read from disk list file");
         }

         LITTLE_ENDIAN_TO_HOST_64(itemLen, itemLen);

         boost::scoped_array<char> itemBuf(new char[itemLen]);

         ssize_t readRes = ::pread(fd, itemBuf.get(), itemLen, offset + sizeof(itemLen));

         if (readRes < (ssize_t) itemLen)
            throw std::runtime_error("could not read from disk list file");

         unsigned len;
         if (!item.deserialize(itemBuf.get(), itemLen, &len))
            throw std::runtime_error("could not read from disk list file");

         offset += sizeof(itemLen) + itemLen;
         return true;
      }

      Data* get()
      {
         return &item;
      }

   private:
      int fd;
      size_t offset;
      Data item;

      DiskListCursor(int fd):
         fd(fd), offset(0)
      {
      }
};

template<typename Data>
class DiskList {
   private:
      std::string file;
      int fd;
      uint64_t itemCount;
      Mutex mtx;

      DiskList(const DiskList&);
      DiskList& operator=(const DiskList&);

   public:
      DiskList(const std::string& file, bool allowCreate = true) :
         file(file), itemCount(0)
      {
         fd = ::open(file.c_str(), O_RDWR | (allowCreate ? O_CREAT : 0), 0660);
         if(fd < 0)
         {
            if(!allowCreate && errno == ENOENT)
               throw DiskListDoesNotExist(file);
            else
               throw std::runtime_error("could not open disk list file");
         }
      }

      ~DiskList()
      {
         if(fd >= 0)
            close(fd);
      }

      const std::string filename() const { return file; }

      void append(Data& data)
      {
         SafeMutexLock lock(&mtx);

         boost::scoped_array<char> buffer(new char[data.serialLen()]);

         size_t size = data.serialize(buffer.get());

         uint64_t bufSize;
         HOST_TO_LITTLE_ENDIAN_64(size, bufSize);
         if (::write(fd, &bufSize, sizeof(bufSize)) < (ssize_t) sizeof(bufSize))
         {
            lock.unlock();
            throw std::runtime_error("error writing disk list item");
         }

         if (::write(fd, buffer.get(), size) < (ssize_t) size)
         {
            lock.unlock();
            throw std::runtime_error("error writing disk list item");
         }

         lock.unlock();
      }

      DiskListCursor<Data> cursor()
      {
         return DiskListCursor<Data>(fd);
      }

      void clear()
      {
         if (fd >= 0)
         {
            if (ftruncate(fd, 0) < 0)
               throw std::runtime_error("error clearing disk list");
         }
      }
};

#endif
