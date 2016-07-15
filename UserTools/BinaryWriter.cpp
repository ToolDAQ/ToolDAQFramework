#include "BinaryWriter.h"

BinaryWriter::BinaryWriter():Tool(){}


bool BinaryWriter::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;

  m_variables.Get("outfile",outfile);

  ofs=new std::ofstream(outfile.c_str());
  //oa=new boost::archive::text_oarchive(*ofs);
  oa=new boost::archive::binary_oarchive(*ofs);
  

  return true;
}


bool BinaryWriter::Execute(){

  for(int i=0;i<m_data->Cards.size();i++){

    *oa << *(m_data->Cards.at(i));

  }

  return true;
}


bool BinaryWriter::Finalise(){

  delete oa;
  oa=0;
  delete ofs;
  ofs=0;

  
  for(int i=0;i<m_data->Cards.size();i++){
    
    delete  m_data->Cards.at(i);

  }
  
  m_data->Cards.clear();
  


  return true;

}
