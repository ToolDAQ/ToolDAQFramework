#include "BoardReader.h"

BoardReader::BoardReader():Tool(){}


bool BoardReader::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;

  int seed=0;
  m_variables.Get("seed",seed);
  srand(seed);

  m_variables.Get("channels",Channels);
  m_variables.Get("cardID",CardID);

  return true;
}


bool BoardReader::Execute(){

  CardData* cd=new CardData;

  cd->Channels=Channels;
  cd->CardID=CardID;
  
  for(int i=0;i<Channels;i++){
    cd->Energy.push_back(rand() %100);
    cd->Time.push_back(std::time(NULL));
    cd->Channel.push_back(i);
  }

  m_data->Cards.push_back(cd);
  
  return true;
}


bool BoardReader::Finalise(){

  return true;
}
