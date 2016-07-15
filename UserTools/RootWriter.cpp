#include "RootWriter.h"

RootWriter::RootWriter():Tool(){}


bool RootWriter::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;

  std::string outpath; 
  m_variables.Get("outpath",outpath);
  out=new TFile(outpath.c_str(),"RECREATE");

  TTree * tree=new TTree("Raw","Raw");

  tree->Branch("eventnum",&m_data->eventnum,"eventnum/L");
  tree->Branch("Channels",&localcard.Channels,"Channels/I");
  tree->Branch("Energy",&localcard.Energy);
  tree->Branch("Time",&localcard.Time);
  tree->Branch("Channel",&localcard.Channel);


  m_data->AddTTree("Raw",tree);

  m_data->eventnum=0;

  return true;

}


bool RootWriter::Execute(){

  m_data->eventnum+=1;

  for(int card=0;card<m_data->Cards.size();card++){
    
    localcard.Channels=m_data->Cards.at(card)->Channels;
    localcard.Energy.clear();
    localcard.Energy=m_data->Cards.at(card)->Energy;
    localcard.Time.clear();
    localcard.Time=m_data->Cards.at(card)->Time;
    localcard.Channel.clear();
    localcard.Channel=m_data->Cards.at(card)->Channel;
 
    delete m_data->Cards.at(card);
      m_data->Cards.at(card)=0;
    
  }
  
  m_data->GetTTree("Raw")->Fill();

  m_data->Cards.clear();
  
  return true;
}


bool RootWriter::Finalise(){

  m_data->GetTTree("Raw")->Write();
  
  out->Close();
  
  m_data->DeleteTTree("Raw");
    
  delete out;
  out=0;
  
  return true;

}
