#ifndef MODESETMETADATAMIRRORING_H_
#define MODESETMETADATAMIRRORING_H_

#include <common/Common.h>
#include <modes/Mode.h>

class ModeSetMetadataMirroring : public Mode
{
   public:
      ModeSetMetadataMirroring() {}
      
      virtual int execute();
   
      static void printHelp();

      
   private:

      bool setMirroring(Node* ownerNode, EntryInfo* entryInfo);
   
};

#endif /*MODESETMETADATAMIRRORING_H_*/
