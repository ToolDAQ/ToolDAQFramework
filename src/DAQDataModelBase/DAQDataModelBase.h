#ifndef DAQDATAMODELBASE_H
#define DAQDATAMODELBASE_H

#include <map>
#include <string>
#include <vector>

//#include "TTree.h"

#include "Store.h"
#include "BoostStore.h"
#include "DAQLogging.h"
#include "DAQUtilities.h"
#include "SlowControlCollection.h"
#include "DataModelBase.h"
#include "Services.h"


#include <zmq.hpp>

namespace ToolFramework{
  
  /**
   * \class DataModel
   *
   * This class Is a transient data model class for your Tools within the ToolChain. If Tools need to comunicate they pass all data objects through the data model. There fore inter tool data objects should be deffined in this class. 
   *
   *
   * $Author: B.Richards $ 
   * $Date: 2019/05/26 18:34:00 $
   *
   */
  
  class DAQDataModelBase : public DataModelBase {
    
    
  public:
    
    DAQDataModelBase(); ///< Simple constructor 
    
    //TTree* GetTTree(std::string name);
    //void AddTTree(std::string name,TTree *tree);
    //void DeleteTTree(std::string name,TTree *tree);
    
    BoostStore CStore; ///< This is a more efficent binary BoostStore that can be used to store a dynamic set of inter Tool variables.
    std::map<std::string,BoostStore*> Stores; ///< This is a map of named BooStore pointers which can be deffined to hold a nammed collection of any tipe of BoostStore. It is usefull to store data that needs subdividing into differnt stores.
    
    
    zmq::context_t* context; ///< ZMQ contex used for producing zmq sockets for inter thread,  process, or computer communication
    
    SlowControlCollection sc_vars; ///< calss for defining and handelling slow control variables

    Services* services;
    
    //  bool (*Log)(std::string, int);
    
    /*  
	template<Type T>
	struct Log {
	typedef bool (*type)(T message,int verboselevel);
	};
    */
  private:
    
    
    
    //std::map<std::string,TTree*> m_trees; 
    
    
    
  };
  
}

#endif
