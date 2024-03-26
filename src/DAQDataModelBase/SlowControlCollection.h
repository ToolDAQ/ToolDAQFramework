#ifndef SLOW_CONTROL_COLLECTION_H
#define SLOW_CONTROL_COLLECTION_H

#include <SlowControlElement.h>
#include <zmq.hpp>
#include "DAQUtilities.h"
#include <functional>

namespace ToolFramework{
  
  class SlowControlCollection;
  
  struct SlowControlCollectionThread_args:Thread_args{
    
    SlowControlCollectionThread_args();
    ~SlowControlCollectionThread_args();
    
    zmq::socket_t* sock;
    zmq::socket_t* sub;
    zmq::pollitem_t items[2];
    int poll_length;
    
    SlowControlCollection* SCC;
    std::map<std::string, std::function<void(const char*, const char*)> >* alert_functions;
    std::mutex* alert_functions_mutex;
    
  };
  
  class SlowControlCollection{
    
   public:
    
    SlowControlCollection();
    ~SlowControlCollection();
    
    bool Init(zmq::context_t* context, int port=555, bool new_service=true);
    bool ListenForData(int poll_length=0);
    bool InitThreadedReceiver(zmq::context_t* context, int port=555, int poll_length=100, bool new_service=true);
    SlowControlElement* operator[](std::string key);
    bool Add(std::string name, SlowControlElementType type, std::function<std::string(const char*)> function=nullptr);
    bool Remove(std::string name);
    void Clear();
    bool AlertSubscribe(std::string alert, std::function<void(const char*, const char*)> function);
    bool AlertSend(std::string alert, std::string="");
    std::string Print();
    
    template<typename T> T GetValue(std::string name){
      return SC_vars[name]->GetValue<T>();
    }
    
    
   private:
    
    std::map<std::string, SlowControlElement*> SC_vars;
    std::map<std::string, std::function<void(const char*, const char*)> > m_alert_functions;
    std::mutex m_alert_functions_mutex;
    
    DAQUtilities* m_util;
    zmq::context_t* m_context;
    zmq::socket_t* m_pub;
    SlowControlCollectionThread_args* args;
    
    static void Thread(Thread_args* arg);
    
  };
  
}

#endif
