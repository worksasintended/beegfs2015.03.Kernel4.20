#include <app/config/MountConfig.h>
#include <common/Common.h>

#include <linux/parser.h>


enum {
   /* Mount options that take string arguments */
   Opt_cfgFile,
   Opt_logStdFile,
   Opt_logErrFile,
   Opt_sysMgmtdHost,
   Opt_tunePreferredMetaFile,
   Opt_tunePreferredStorageFile,

   /* Mount options that take integer arguments */
   Opt_logLevel,
   Opt_connPortShift,
   Opt_connMgmtdPortUDP,
   Opt_connMgmtdPortTCP,
   Opt_sysMountSanityCheckMS,

   Opt_err
};


static match_table_t fhgfs_mount_option_tokens =
{
   /* Mount options that take string arguments */
   { Opt_cfgFile, "cfgFile=%s" },
   { Opt_logStdFile, "logStdFile=%s" },
   { Opt_logErrFile, "logErrFile=%s" },
   { Opt_sysMgmtdHost, "sysMgmtdHost=%s" },
   { Opt_tunePreferredMetaFile, "tunePreferredMetaFile=%s" },
   { Opt_tunePreferredStorageFile, "tunePreferredStorageFile=%s" },

   /* Mount options that take integer arguments */
   { Opt_logLevel, "logLevel=%d" },
   { Opt_connPortShift, "connPortShift=%d" },
   { Opt_connMgmtdPortUDP, "connMgmtdPortUDP=%u" },
   { Opt_connMgmtdPortTCP, "connMgmtdPortTCP=%u" },
   { Opt_sysMountSanityCheckMS, "sysMountSanityCheckMS=%u" },

   { Opt_err, NULL }
};



fhgfs_bool MountConfig_parseFromRawOptions(MountConfig* this, char* mountOptions)
{
   char* currentOption;

   if(!mountOptions)
   {
      printk_fhgfs_debug(KERN_INFO, "Mount options = <none>\n");
      return fhgfs_true;
   }


   printk_fhgfs_debug(KERN_INFO, "Mount options = '%s'\n", mountOptions);

   while( (currentOption = strsep(&mountOptions, ",") ) != NULL)
   {
      substring_t args[MAX_OPT_ARGS];
      int tokenID;

      if(!*currentOption)
         continue; // skip empty string

      tokenID = match_token(currentOption, fhgfs_mount_option_tokens, args);

      switch(tokenID)
      {
         /* Mount options that take STRING arguments */

         case Opt_cfgFile:
         {
            SAFE_KFREE(this->cfgFile);

            this->cfgFile = match_strdup(args);// (string kalloc'ed => needs kfree later)
         } break;

         case Opt_logStdFile:
         {
            SAFE_KFREE(this->logStdFile);

            this->logStdFile = match_strdup(args); // (string kalloc'ed => needs kfree later)
         } break;

         case Opt_logErrFile:
         {
            SAFE_KFREE(this->logErrFile);

            this->logErrFile = match_strdup(args); // (string kalloc'ed => needs kfree later)
         } break;

         case Opt_sysMgmtdHost:
         {
            SAFE_KFREE(this->sysMgmtdHost);

            this->sysMgmtdHost = match_strdup(args); // (string kalloc'ed => needs kfree later)
         } break;

         case Opt_tunePreferredMetaFile:
         {
            SAFE_KFREE(this->tunePreferredMetaFile);

            this->tunePreferredMetaFile = match_strdup(args); // (string kalloc'ed => needs kfree later)
         } break;

         case Opt_tunePreferredStorageFile:
         {
            SAFE_KFREE(this->tunePreferredStorageFile);

            this->tunePreferredStorageFile = match_strdup(args); // (string kalloc'ed => needs kfree later)
         } break;

         /* Mount options that take INTEGER arguments */

         case Opt_logLevel:
         {
            if(match_int(args, &this->logLevel) )
               goto err_exit_invalid_option;

            this->logLevelDefined = fhgfs_true;
         } break;

         case Opt_connPortShift:
         {
            if(match_int(args, &this->connPortShift) )
               goto err_exit_invalid_option;

            this->connPortShiftDefined = fhgfs_true;
         } break;

         case Opt_connMgmtdPortUDP:
         {
            if(match_int(args, &this->connMgmtdPortUDP) )
               goto err_exit_invalid_option;

            this->connMgmtdPortUDPDefined = fhgfs_true;
         } break;

         case Opt_connMgmtdPortTCP:
         {
            if(match_int(args, &this->connMgmtdPortTCP) )
               goto err_exit_invalid_option;

            this->connMgmtdPortTCPDefined = fhgfs_true;
         } break;

         case Opt_sysMountSanityCheckMS:
         {
            if(match_int(args, &this->sysMountSanityCheckMS) )
               goto err_exit_invalid_option;

            this->sysMountSanityCheckMSDefined = fhgfs_true;
         } break;

         default:
            goto err_exit_unknown_option;
      }
   }

   return fhgfs_true;


err_exit_unknown_option:
   printk_fhgfs(KERN_WARNING, "Unknown mount option: '%s'\n", currentOption);
   return fhgfs_false;

err_exit_invalid_option:
   printk_fhgfs(KERN_WARNING, "Invalid mount option: '%s'\n", currentOption);
   return fhgfs_false;
}



