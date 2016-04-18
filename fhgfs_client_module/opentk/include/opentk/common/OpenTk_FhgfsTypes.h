#ifndef OPENTK_FHGFSTYPES_H_
#define OPENTK_FHGFSTYPES_H_

typedef bool fhgfs_bool;
#define fhgfs_false  false
#define fhgfs_true   true


struct fhgfs_timespec
{
   time_t tv_sec; // seconds
   long   tv_nsec;  // nanoseconds
};
typedef struct fhgfs_timespec fhgfs_timespec;


#endif /* OPENTK_FHGFSTYPES_H_ */
