#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <map>
#include <string>
#include <vector>

//#include "TTree.h"

#include "Store.h"
#include "Logging.h"

#include <zmq.hpp>

/**
 * \class DataModel
 *
 *
 * This class Is a transient data model class for your Tools within the ToolChain. If Tools need to comunicate they pass all data objects through the data model. There fore inter tool data objects should be deffined in this class. 
 *
 *
 * $Author: B.Richards $
 *
 * $Date: 2019/05/26 18:34:00 $
 *
 * Contact: b.richards@qmul.ac.uk
 *
 *
 */

class DataModel {


 public:
  
  DataModel(); ///< Simple constructor 

  //TTree* GetTTree(std::string name);
  //void AddTTree(std::string name,TTree *tree);
  //void DeleteTTree(std::string name,TTree *tree);

  Store vars; ///< This store can be used for any variables. It is an inefficent ascii based storage
  Logging *Log; ///< Log class pointer for use in tools, it can be used to send messages which can have multiple error levels and destination end points

  zmq::context_t* context; ///< ZMQ contex used for producing zmq sockets for inter thread,  process, or computer communication


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



#endif
