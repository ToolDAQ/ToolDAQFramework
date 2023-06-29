#ifndef SLOW_CONTROL_COLLECTION_H
#define SLOW_CONTROL_COLLECTION_H

#include <SlowControlElement.h>
#include <zmq.hpp>
#include "DAQUtilities.h"

class SlowControlCollection;

struct SlowControlCollectionThread_args:Thread_args{

  SlowControlCollectionThread_args();
  ~SlowControlCollectionThread_args();

  zmq::socket_t* sock;
  zmq::pollitem_t items[1];
  int poll_length;

  SlowControlCollection* SCC;
  
};

class SlowControlCollection{

 public:

  SlowControlCollection();
  ~SlowControlCollection();

  bool Init(zmq::context_t* context, int port=555, bool new_service=true);
  bool ListenForData(int poll_length=0);
  bool InitThreadedReceiver(zmq::context_t* context, int port=555, int poll_length=100, bool new_service=true);
  SlowControlElement* operator[](std::string key);
  bool Add(std::string name, SlowControlElementType type);
  bool Remove(std::string name);
  void Clear();
  std::string Print();
  template<typename T> T GetValue(std::string name){

    return SC_vars[name]->GetValue<T>();    

  }

 private:

  std::map<std::string, SlowControlElement*> SC_vars;
  
  DAQUtilities* m_util;
  zmq::context_t* m_context;
  SlowControlCollectionThread_args* args;

  static void Thread(Thread_args* arg);

};

#endif
