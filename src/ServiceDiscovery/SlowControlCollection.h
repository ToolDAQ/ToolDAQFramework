#ifndef SLOW_CONTROL_COLLECTION_H
#define SLOW_CONTROL_COLLECTION_H

#include <SlowControlElement.h>
#include <zmq.hpp>
#include "DAQUtilities.h"
#include <functional>

namespace ToolFramework{

  //typedef void (*AlertFunction)(const char*, const char*);
  typedef std::function<void(const char*, const char*)> AlertFunction;
  
  class SlowControlCollection;
  
  struct SlowControlCollectionThread_args:Thread_args{
    
    SlowControlCollectionThread_args();
    ~SlowControlCollectionThread_args();
    
    zmq::socket_t* sock;
    zmq::socket_t* sub;
    zmq::pollitem_t items[2];
    int poll_length;
    
    SlowControlCollection* SCC;
    std::map<std::string, AlertFunction>* alert_functions;
    std::mutex* alert_functions_mutex;
    bool alerts_receive;
    
  };
  
  class SlowControlCollection{
    
   public:
    
    SlowControlCollection();
    ~SlowControlCollection();
    
    bool Init(zmq::context_t* context, int port=60000, bool new_service=true, int alert_receive_port=12243, bool alert_receive=true, int alert_send_port=12242, bool alert_send=true);
    bool ListenForData(int poll_length=0);
    bool InitThreadedReceiver(zmq::context_t* context, int port=60000, int poll_length=100, bool new_service=true, int alert_receive_port=12243, bool alert_receive=true, int alert_send_port=12242, bool alert_send=true);
    SlowControlElement* operator[](std::string key);
    bool Add(std::string name, SlowControlElementType type, SCFunction change_function = 0, SCFunction read_function = 0 );
    bool Remove(std::string name);
    void Clear();
    bool AlertSubscribe(std::string alert, AlertFunction function);
    bool AlertSend(std::string alert, std::string="");
    std::string Print();
    std::string PrintJSON();
    void Stop();
    void JsonParser(std::string json);
    
    
    template<typename T> T GetValue(std::string name){
      if(!SC_vars.count(name)) return T{};
      return SC_vars[name]->GetValue<T>();
    }
    
    
   private:
    
    std::map<std::string, SlowControlElement*> SC_vars;
    std::map<std::string, AlertFunction>  m_alert_functions;
    std::mutex m_alert_functions_mutex;
    
    DAQUtilities* m_util;
    zmq::context_t* m_context;
    zmq::socket_t* m_pub;
    SlowControlCollectionThread_args* args;
    bool m_new_service;
    bool m_thread;
    bool m_alerts_receive;
    bool m_alerts_send;
    
    static void Thread(Thread_args* arg);
    void Unpack(std::string in, std::map<std::string,std::string> &out, std::string header="");
    bool Update(std::string key, std::string value, std::string &reply, bool &strip);
    static bool Update( SlowControlCollection* SCC, std::string key, std::string value, std::string &reply, bool &strip);
    
  };
  
}

#endif
