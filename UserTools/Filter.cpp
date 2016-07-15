#include "Filter.h"

Filter::Filter():Tool(){}


bool Filter::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;

  m_variables.Get("threshold",threshold);

  m_data->eventnum=0;

  return true;

}


bool Filter::Execute(){

  std::cout<<"debug 1"<<std::endl;
  m_data->eventnum+=1;
  
  std::vector<int> delcards;
  std::cout<<"debug 2"<<std::endl;
  for(int card=0;card<m_data->Cards.size();card++){
    std::cout<<"debug 3"<<std::endl;
    std::vector<int> delchannels;
    
    for(int channel;channel<m_data->Cards.at(card)->Channels;channel++){
      std::cout<<"debug 4 "<<channel<<std::endl;
      if(m_data->Cards.at(card)->Energy.at(channel)<threshold){
	std::cout<<"debug 4.5 "<<channel<<std::endl;
	delchannels.push_back(channel);
      }
      std::cout<<"debug 5"<<std::endl;    
    }
    std::cout<<"debug 6"<<std::endl;
    for(int i=delchannels.size()-1;i>-1;i--){
      std::cout<<"debug 7"<<std::endl;
      m_data->Cards.at(card)->Energy.erase(m_data->Cards.at(card)->Energy.begin()+delchannels.at(i));
      m_data->Cards.at(card)->Time.erase(m_data->Cards.at(card)->Time.begin()+delchannels.at(i));
      m_data->Cards.at(card)->Channel.erase(m_data->Cards.at(card)->Channel.begin()+delchannels.at(i));
      std::cout<<"debug 8"<<std::endl;    
}
    std::cout<<"debug 9"<<std::endl;
  }
  std::cout<<"debug 10"<<std::endl;
  for(int i=0;i<delcards.size();i++){
    std::cout<<"debug 11"<<std::endl;
    m_data->Cards.erase(m_data->Cards.begin()+delcards.at(i));
  }
  std::cout<<"debug 12"<<threshold<<std::endl;
  
  return true;

}


bool Filter::Finalise(){

  return true;

}
