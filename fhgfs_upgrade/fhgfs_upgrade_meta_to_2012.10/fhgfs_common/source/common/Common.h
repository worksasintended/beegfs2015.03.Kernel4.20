#ifndef COMMON_H_
#define COMMON_H_

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip> // output formating
#include <fstream>
#include <sstream>
#include <pthread.h>
#include <map>
#include <set>
#include <list>
#include <vector>
#include <stdexcept>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cerrno>
#include <unistd.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sys/poll.h>
#include <netdb.h>
#include <algorithm>
#include <sys/time.h>
#include <fcntl.h>


/**
 * connection (response) timeouts in ms
 * note: be careful here, because servers not responding for >30secs during RAID cache flush is
 * nothing unusual, so never use CONN_SHORT_TIMEOUT for IO-related operations.
 */
#define CONN_LONG_TIMEOUT     600000
#define CONN_MEDIUM_TIMEOUT    90000
#define CONN_SHORT_TIMEOUT     30000


typedef std::map<std::string, std::string> StringMap;
typedef StringMap::iterator StringMapIter;
typedef StringMap::const_iterator StringMapCIter;
typedef StringMap::value_type StringMapVal;

typedef std::map<std::string, int64_t> StringInt64Map;
typedef StringInt64Map::iterator StringInt64MapIter;
typedef StringInt64Map::const_iterator StringInt64MapCIter;
typedef StringInt64Map::value_type StringInt64MapVal;

typedef std::map<std::string, int64_t> StringUnsignedMap;
typedef StringUnsignedMap::iterator StringUnsignedMapIter;
typedef StringUnsignedMap::const_iterator StringUnsignedMapCIter;
typedef StringUnsignedMap::value_type StringUnsignedMapVal;

typedef std::set<std::string> StringSet;
typedef StringSet::iterator StringSetIter;
typedef StringSet::const_iterator StringSetCIter;

typedef std::list<std::string> StringList;
typedef StringList::iterator StringListIter;
typedef StringList::const_iterator StringListConstIter;

typedef std::vector<std::string> StringVector;
typedef StringVector::iterator StringVectorIter;
typedef StringVector::const_iterator StringVectorConstIter;

typedef std::set<uint16_t> UInt16Set;
typedef UInt16Set::iterator UInt16SetIter;
typedef UInt16Set::const_iterator UInt16SetCIter;

typedef std::list<uint16_t> UInt16List;
typedef UInt16List::iterator UInt16ListIter;
typedef UInt16List::const_iterator UInt16ListConstIter;

typedef std::vector<uint16_t> UInt16Vector;
typedef UInt16Vector::iterator UInt16VectorIter;
typedef UInt16Vector::const_iterator UInt16VectorConstIter;

typedef std::set<int> IntSet;
typedef IntSet::iterator IntSetIter;
typedef IntSet::const_iterator IntSetCIter;

typedef std::list<int> IntList;
typedef IntList::iterator IntListIter;
typedef IntList::const_iterator IntListConstIter;

typedef std::list<unsigned> UIntList;
typedef UIntList::iterator UIntListIter;
typedef UIntList::const_iterator UIntListConstIter;

typedef std::vector<int> IntVector;
typedef IntVector::iterator IntVectorIter;
typedef IntVector::const_iterator IntVectorConstIter;

typedef std::list<int64_t> Int64List;
typedef Int64List::iterator Int64ListIter;
typedef Int64List::const_iterator Int64ListConstIter;

typedef std::list<uint64_t> UInt64List;
typedef UInt64List::iterator UInt64ListIter;
typedef UInt64List::const_iterator UInt64ListConstIter;

typedef std::vector<int64_t> Int64Vector;
typedef Int64Vector::iterator Int64VectorIter;
typedef Int64Vector::const_iterator Int64VectorConstIter;

typedef std::vector<uint64_t> UInt64Vector;
typedef UInt64Vector::iterator UInt64VectorIter;
typedef UInt64Vector::const_iterator UInt64VectorConstIter;


#define FHGFS_MIN(a, b) \
   ( ( (a) < (b) ) ? (a) : (b) )
#define FHGFS_MAX(a, b) \
   ( ( (a) < (b) ) ? (b) : (a) )


#define SAFE_DELETE(p) \
   do{ if(p) {delete (p); (p)=NULL;} } while(0)
#define SAFE_FREE(p) \
   do{ if(p) {free(p); (p)=NULL;} } while(0)

#define SAFE_DELETE_NOSET(p) \
   do{ if(p) delete (p); } while(0)
#define SAFE_FREE_NOSET(p) \
   do{ if(p) free(p); } while(0)

// typically used for optional out-args
#define SAFE_ASSIGN(destPointer, sourceValue) \
   do{ if(destPointer) {*(destPointer) = (sourceValue);} } while(0)


// gcc branch optimization hints
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x)  __builtin_expect(!!(x), 0)


// this macro mutes warnings about unused variables
// (use this power wisely, my friend!)
#define IGNORE_UNUSED_VARIABLE(a)   do{ if( ((long)a)==1) {} } while(0)


#ifdef FHGFS_DEBUG
   #define FHGFS_DEBUG_PROFILING
#endif

/*
 * Optional definitions:
 * - FHGFS_NO_LICENSE_CHECK: Disables all license checking at startup and runtime
 */

/*
 * Debug definitions:
 * - FHGFS_DEBUG: Generally defined for debug builds. Might turn on some other debug definitions
 *    below automatically.
 *
 * - LOG_DEBUG_MESSAGES: Enables logging of some extra debug messages that will not be
 *    available otherwise
 * - DEBUG_REFCOUNT: Enables debugging of ObjectReferencer::refCount. Error messages will
 *    be logged if refCount is less than zero
 * - DEBUG_MUTEX_LOCKING: Enables the debug functionality of the SafeMutexLock class
 * - FHGFS_DEBUG_PROFILING: Additional timestamps in log messages.
 */


#endif /*COMMON_H_*/
