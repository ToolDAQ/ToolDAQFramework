#include "Filter.h"

Filter::Filter():Tool(){}


bool Filter::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;

  return true;
}


bool Filter::Execute(){

  return true;
}


bool Filter::Finalise(){

  return true;
}
