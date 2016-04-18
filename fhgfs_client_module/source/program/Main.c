#include <common/Common.h>
#include <program/Program.h>

#define BEEGFS_LICENSE "GPL v2"


static int __init init_fhgfs_client(void)
{
   if(!Program_main() )
      return 0;

   return -EPERM;
}

static void __exit exit_fhgfs_client(void)
{
   Program_exit();
}

module_init(init_fhgfs_client)
module_exit(exit_fhgfs_client)

MODULE_LICENSE(BEEGFS_LICENSE);
MODULE_DESCRIPTION("BeeGFS parallel file system client (http://www.beegfs.com)");
MODULE_AUTHOR("Fraunhofer ITWM, CC-HPC");
