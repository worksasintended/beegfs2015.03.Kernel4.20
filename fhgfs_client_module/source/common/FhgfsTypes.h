#ifndef OPEN_FHGFSTYPES_H_
#define OPEN_FHGFSTYPES_H_

#include <opentk/common/OpenTk_FhgfsTypes.h>


#define BEEGFS_LLONG_MAX    ((long long)(~0ULL>>1))


typedef unsigned fhgfs_in_addr;


struct fhgfs_sockaddr_in
{
   fhgfs_in_addr addr;
   unsigned short port; // port in network byte order
};
typedef struct fhgfs_sockaddr_in fhgfs_sockaddr_in;

#define BEEGFS_INADDR_ANY        ( (unsigned long int) 0x00000000) // accept any address
#define BEEGFS_INADDR_BROADCAST  ( (unsigned long int) 0xffffffff) // broadcast address
#define BEEGFS_INADDR_LOOPBACK   ( (unsigned long int) 0x7f000001) // 127.0.0.1 (host byte order)
#define BEEGFS_INADDR_NONE       ( (unsigned long int) 0xffffffff) // error address



struct fhgfs_timeval
{
   time_t      tv_sec;  // seconds
   suseconds_t tv_usec; // microseconds
};
typedef struct fhgfs_timeval fhgfs_timeval;


struct fhgfs_stat
{
   umode_t mode;
   unsigned int nlink;
   uid_t uid;
   gid_t gid;
   loff_t size;
   uint64_t blocks;
   fhgfs_timespec atime;
   fhgfs_timespec mtime;
   fhgfs_timespec ctime; // attrib change time (not creation time)
};
typedef struct fhgfs_stat fhgfs_stat;



#endif /* OPEN_FHGFSTYPES_H_ */
