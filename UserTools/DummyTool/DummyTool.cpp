#include "DummyTool.h"

DummyTool::DummyTool():Tool(){}


bool DummyTool::Initialise(std::string configfile, DataModel &data){

  InitialiseTool(data);
  InitialiseConfiguration(configfile);

  //m_variables.Print();

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;
 
  Log("test 1",1);

  ExportConfiguration();

  return true;
}


bool DummyTool::Execute(){
  	
  // example of print out methods 
  // mesage level indicates the minimum verbosity level to print out a message
  // Therefore a message level of 0 is always printed so should be used for high priority messages e.g. errors
  // and a message level or 9 would be for minor messgaes rarely printed

 Log("test 2a"); // defualt log function message level is 0 
 *m_log<<"test 2b"<<std::endl; // defualt stream message level is 0 

 *m_log<<"m_verbose = "<<m_verbose<<std::endl; // Prints current verbosity level

  // To create a printout for a higher level can do the following
 Log("test 3a",2); // will only print if m_verbose is equal to or higher than 2
 *m_log<<ML(2)<<"test 3b"<<std::endl; // note: steam message level persists and is not automatically reset
  
 *m_log<<"test 4"<<std::endl; // Still message level 2

 // This can be fixed by a message level clear function MLC();
 MLC();  // call message level clear function;
 *m_log<<"test 5"<<std::endl; // back to message level 0

 // whilst it's possible to change the value of m_verbose for the tool as a whole 
 // you can also manually change it for a single message call if needed (not best practice so avoid if possible)
 int tmp_verbosity=4;
 Log("test 6a",2,tmp_verbosity);
 *m_log<<MsgL(2,m_verbose)<<"test 6b"<<std::endl;
 MLC();

 // logging stream also allows for an array of colours which will automatically clear after ending the line
 *m_log<<red<<"red, "<<"still red, "<<green<<"green, "<<plain<<"no colour, "<<purple<<"purple"<<std::endl;
 *m_log<<"no colour"<<std::endl;


  return true;
}


bool DummyTool::Finalise(){

  Log("test 11",1);

  return true;
}
