#include "fault-inject.h"

#include <linux/version.h>

#if defined(BEEGFS_DEBUG) && defined(CONFIG_FAULT_INJECTION)

static struct dentry* debug_dir;
static struct dentry* fault_dir;

#define BEEGFS_DECLARE_FAULT_ATTR(name) DECLARE_FAULT_ATTR(beegfs_fault_ ## name)

BEEGFS_DECLARE_FAULT_ATTR(readpage);
BEEGFS_DECLARE_FAULT_ATTR(writepage);

#define INIT_FAULT(name) \
   if(IS_ERR(fault_create_debugfs_attr(#name, fault_dir, &beegfs_fault_ ## name))) \
      goto err_fault_dir;

#ifndef KERNEL_HAS_FAULTATTR_DNAME
#define DESTROY_FAULT(name) do { } while (0)
#else
#define DESTROY_FAULT(name) \
   do { \
      if((beegfs_fault_ ## name).dname) \
         dput( (beegfs_fault_ ## name).dname); \
   } while (0)
#endif

bool beegfs_fault_inject_init()
{
   debug_dir = debugfs_create_dir("beegfs", NULL);
   if(!debug_dir)
      goto err_debug_dir;

   fault_dir = debugfs_create_dir("fault", debug_dir);
   if(!fault_dir)
      goto err_fault_dir;

   INIT_FAULT(readpage);
   INIT_FAULT(writepage);

   return true;

err_fault_dir:
   debugfs_remove_recursive(debug_dir);
   debug_dir = NULL;
err_debug_dir:
   return false;
}

void beegfs_fault_inject_release()
{
   DESTROY_FAULT(readpage);
   DESTROY_FAULT(writepage);

   debugfs_remove_recursive(debug_dir);
}

#endif
