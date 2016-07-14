#include "RootWriter.h"

RootWriter::RootWriter():Tool(){}


bool RootWriter::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;

  return true;
}


bool RootWriter::Execute(){

  return true;
}


bool RootWriter::Finalise(){

  return true;
}
