#include "BoardReader.h"

BoardReader::BoardReader():Tool(){}


bool BoardReader::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;

  int seed=0;
  m_variables("seed",seed);
  srand(seed);

  return true;
}


bool BoardReader::Execute(){

  m_data->Energy=rand() %100;
  m_data->Time=std::time(NULL);


  return true;
}


bool BoardReader::Finalise(){

  return true;
}
