#ifndef SLOW_CONTROL_COLLECTION_H
#define SLOW_CONTROL_COLLECTION_H

#include <SlowControlElement.h>
#include <zmq.hpp>
#include "DAQUtilities.h"
#include <functional>

class SlowControlCollection;

struct SlowControlCollectionThread_args:Thread_args{

  SlowControlCollectionThread_args();
  ~SlowControlCollectionThread_args();

  zmq::socket_t* sock;
  zmq::socket_t* sub;
  zmq::pollitem_t items[2];
  int poll_length;

  SlowControlCollection* SCC;
  std::map<std::string, std::function<void(std::string)> >* trigger_functions;  
  std::mutex* trigger_functions_mutex;

};

class SlowControlCollection{

 public:

  SlowControlCollection();
  ~SlowControlCollection();

  bool Init(zmq::context_t* context, int port=555, bool new_service=true);
  bool ListenForData(int poll_length=0);
  bool InitThreadedReceiver(zmq::context_t* context, int port=555, int poll_length=100, bool new_service=true);
  SlowControlElement* operator[](std::string key);
  bool Add(std::string name, SlowControlElementType type, std::function<std::string(std::string)> function=nullptr);
  bool Remove(std::string name);
  void Clear();
  bool TriggerSubscribe(std::string trigger, std::function<void(std::string)> function);
  bool TriggerSend(std::string trigger);
  std::string Print();
  template<typename T> T GetValue(std::string name){

    return SC_vars[name]->GetValue<T>();    

  }
  

 private:

  std::map<std::string, SlowControlElement*> SC_vars;
  std::map<std::string, std::function<void(std::string)> > m_trigger_functions;
  std::mutex m_trigger_functions_mutex;
  
  DAQUtilities* m_util;
  zmq::context_t* m_context;
  zmq::socket_t* m_pub;
  SlowControlCollectionThread_args* args;

  static void Thread(Thread_args* arg);

};

#endif
