#include <string>
#include "ToolDAQChain.h"
#include "DataModel.h"
//#include "DummyTool.h"

using namespace ToolFramework;

int main(int argc, char* argv[]){

  std::string conffile;
  if (argc==1)conffile="configfiles/Dummy/ToolChainConfig";
  else conffile=argv[1];

  DataModel* data_model = new DataModel();
  ToolDAQChain tools(conffile, data_model, argc, argv);


  //DummyTool dummytool;    

  //tools.Add("DummyTool",&dummytool,"configfiles/DummyToolConfig");

  //int portnum=24000;
  //  tools.Remote(portnum);
  //tools.Interactive();
  
  //  tools.Initialise();
  // tools.Execute();
  //tools.Finalise();
  
  
  return 0;
  
}
