#ifndef OPEN_OSDEPS_H_
#define OPEN_OSDEPS_H_

#include <filesystem/FhgfsOps_versions.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>


#define os_strncasecmp(s1, s2, n)   os_strnicmp(s1, s2, n)



struct inode; // forward declaration


extern fhgfs_bool os_checkCompileTimeAssumptions(void);

extern int os_printk(const char* fmt, ...);
extern int os_sprintf(char* buf, const char* fmt, ...);
extern int os_snprintf(char* buf, size_t size, const char* fmt, ...);
extern int os_vsnprintf(char* buf, size_t size, const char* fmt, va_list args);
extern int os_scnprintf(char* buf, size_t size, const char* fmt, ...);
extern int os_sscanf(const char* buf, const char* fmt, ...);

#ifdef BEEGFS_DEBUG
   extern void* os_saveStackTrace(void);
   extern void os_printStackTrace(void * trace, int spaces);
   extern void os_freeStackTrace(void *trace);
#endif // BEEGFS_DEBUG


// inliners
static inline void* os_kmalloc(size_t size);
static inline void* os_kzalloc(size_t size);
static inline void os_kfree(const void* addr);
static inline void* os_vmalloc(unsigned long size);
static inline void os_vfree(void* addr);
static inline unsigned long __os_get_free_page(void);
static inline void os_free_page(unsigned long addr);

static inline void* os_memcpy(void* to, const void* from, size_t n);
static inline void* os_memset(void* s, int c, size_t n);
static inline int os_memcmp(const void* cs, const void* ct, size_t count);

static inline size_t os_strlen(const char* s);
static inline int os_strcmp(const char* s1, const char* s2);
static inline int os_strnicmp(const char* s1, const char* s2, size_t n);
static inline char* os_strcpy(char* dest, const char* src);
static inline char* os_strchr(const char* s,int c);
static inline char* os_strrchr(const char* s, int c);

static inline void* os_getCurrent(void);
static inline const char* os_getCurrentCommNameStr(void);
static inline pid_t os_getCurrentPID(void);

static inline int os_getNumOnlineCPUs(void);

static inline void os_dumpStack(void);

static inline struct inode* os_igrab(struct inode* ino);
static inline void os_ihold(struct inode* ino);

static inline void os_iput(struct inode* ino);

static inline int __os_copy_from_user(void *dst, const void __user *src, unsigned size);
static inline int __os_copy_to_user(void __user *dst, const void *src, unsigned size);
static inline unsigned long __os_clear_user(void __user *mem, unsigned long len);


void* os_kmalloc(size_t size)
{
   void* buf = kmalloc(size, GFP_NOFS);

   if(unlikely(!buf) )
   {
      printk(KERN_WARNING BEEGFS_MODULE_NAME_STR ": kmalloc of '%d' bytes failed. Retrying...\n", (int)size);
      buf = kmalloc(size, GFP_NOFS | __GFP_NOFAIL);
      printk(KERN_WARNING BEEGFS_MODULE_NAME_STR ": kmalloc retry of '%d' bytes succeeded\n", (int)size);
   }

   return buf;
}

void* os_kzalloc(size_t size)
{
   void* buf = kzalloc(size, GFP_NOFS);

   if(unlikely(!buf) )
   {
      printk(KERN_WARNING BEEGFS_MODULE_NAME_STR ": kzalloc of '%d' bytes failed. Retrying...\n", (int)size);
      buf = kzalloc(size, GFP_NOFS | __GFP_NOFAIL);
      printk(KERN_WARNING BEEGFS_MODULE_NAME_STR ": kzalloc retry of '%d' bytes succeeded\n", (int)size);
   }

   return buf;
}

void os_kfree(const void* addr)
{
   kfree(addr);
}

void* os_vmalloc(unsigned long size)
{
   return vmalloc(size);
}

void os_vfree(void* addr)
{
   vfree(addr);
}

unsigned long __os_get_free_page(void)
{
   return __get_free_page(GFP_NOFS);
}

void os_free_page(unsigned long addr)
{
   free_page(addr);
}

void* os_memcpy(void* to, const void* from, size_t n)
{
   return memcpy(to, from, n);
}

void* os_memset(void* s, int c, size_t n)
{
   return memset(s, c, n);
}

int os_memcmp(const void* cs, const void* ct, size_t count)
{
   return memcmp(cs, ct, count);
}

int __os_copy_from_user(void *dst, const void __user *src, unsigned size)
{
   return __copy_from_user(dst, src, size);
}

int __os_copy_to_user(void __user *dst, const void *src, unsigned size)
{
   return __copy_to_user(dst, src, size);
}

/**
 * Zero a block of memory in user space (with less checking).
 */
unsigned long __os_clear_user(void __user *mem, unsigned long len)
{
   return __clear_user(mem, len);
}

size_t os_strlen(const char* s)
{
   return strlen(s);
}

int os_strcmp(const char* s1, const char* s2)
{
   return strcmp(s1, s2);
}

int os_strnicmp(const char *s1, const char *s2, size_t n)
{
   #ifdef KERNEL_HAS_STRNICMP
      return strnicmp(s1, s2, n);
   #else
      return strncasecmp(s1, s2, n);
   #endif
}

char* os_strcpy(char* dest, const char* src)
{
   return strcpy(dest, src);
}

char* os_strchr(const char* s, int c)
{
   return strchr(s, c);
}

char* os_strrchr(const char* s, int c)
{
   return strrchr(s, c);
}

void* os_getCurrent(void)
{
   return get_current();
}

const char* os_getCurrentCommNameStr(void)
{
   return current->comm;
}

pid_t os_getCurrentPID(void)
{
   return current->pid;
}

int os_getNumOnlineCPUs(void)
{
   return num_online_cpus();
}

void os_dumpStack(void)
{
   dump_stack();
}

/**
 * Reference an inode (=> increase usage count).
 *
 * @return on success, the given inode is returned; on error (e.g. if the inode is currently
 * being free'd already) NULL is returned.
 */
struct inode* os_igrab(struct inode* ino)
{
   return igrab(ino);
}

void os_ihold(struct inode* ino)
{
   return ihold(ino);
}

/**
 * Drop inode (=> decrease usage count).
 */
void os_iput(struct inode* ino)
{
   return iput(ino);
}



#endif /* OPEN_OSDEPS_H_ */
