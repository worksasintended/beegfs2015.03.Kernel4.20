#include <common/toolkit/BuildTypeTk.h>
#include "Program.h"

#include <testing/TestRunner.h>

App* Program::app = NULL;

int Program::main(int argc, char** argv)
{
   BuildTypeTk::checkDebugBuildTypes();
   BuildTypeTk::checkHSMBuildTypes();

   AbstractApp::runTimeInitsAndChecks(); // must be called before creating a new App

   app = new App(argc, argv);
   
   app->startInCurrentThread();
   
   int appRes = app->getAppResult();
   
   delete app;
   
   return appRes;
}


