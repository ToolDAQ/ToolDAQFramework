#ifndef ZMQ_MESSAGES_H
#define ZMQ_MESSAGES_H

#include <vector>
#include <iostream>
#include <SerialisableObject.h>
#include <zmq.hpp>

namespace ToolFramework{

  class ZMQMessages: SerialisableObject{
    
  public:
    
    bool Print(){
      return true;
    }
    
    std::string GetVersion(){ return "1";}
    
    bool Serialise(BinaryStream& bs){
      
      
      size_t tmp=messages.size();
      bs << tmp;
      
      for(size_t i=0; i<tmp; i++){
	tmp=messages.at(i).size();
	bs<<tmp;
	bs.Bwrite(messages.at(i).data(), messages.at(i).size());
      }
      
      return true;
    }
    
    std::vector<zmq::message_t> messages;
    bool* sent = 0;
    bool* error = 0;
    
    
  };
  
}
#endif
  
