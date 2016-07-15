#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <map>
#include <string>
#include <vector>

#include "TTree.h"

#include "Store.h"
#include "Logging.h"

#include <zmq.hpp>

#include "CardData.h"

class DataModel {


 public:
  
  DataModel();
  TTree* GetTTree(std::string name);
  void AddTTree(std::string name,TTree *tree);
  void DeleteTTree(std::string name);
  
  Store vars;
  Logging *Log;
  
  zmq::context_t* context;

  std::vector<CardData*> Cards; 
  long eventnum;

 private:
  
  std::map<std::string,TTree*> m_trees; 
   
  
};



#endif
