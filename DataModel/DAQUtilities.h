#ifndef DAQUTILITIES_H
#define DAQUTILITIES_H

#include <iostream>
#include <zmq.hpp>
#include <sstream>
#include <pthread.h>
#include <map>
#include <Store.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <Utilities.h>

/**
 * \struct DAQThread_args
 *
 * This is both an base class for any thread argument struct used in the tool threaded Tool templates.
Effectivly this acts as a place to put variable that are specfic to that thread and can be used as a place to transfer variables from the main thread to sub threads.
 *
 *
 * $Author: B.Richards $
 * $Date: 2019/05/26 18:34:00 $
 * Contact: b.richards@qmul.ac.uk
 *
 */

struct DAQThread_args : public Thread_args{

  DAQThread_args(){ ///< Simple constructor 
    kill=false;
  }

  DAQThread_args(zmq::context_t* contextin, std::string threadname,  void (*funcin)(std::string)){ ///< Construtor for thread with string
   
    context=contextin;
    ThreadName=threadname;
    func_with_string=funcin;
    kill=false; 
  }

  DAQThread_args(zmq::context_t* contextin, std::string threadname,  void (*funcin)(Thread_args*)){ ///< Constrcutor for thread with args
    
    context=contextin;
    ThreadName=threadname;
    func=funcin;
    kill=false;
  }      

  virtual ~DAQThread_args(){ ///< virtual constructor 
    running =false;
    kill=true;
    delete sock;
    sock=0;
  }

  zmq::context_t *context; ///< ZMQ context used for ZMQ socket creation
  void (*func_with_string)(std::string); ///< function pointer to string thread
  zmq::socket_t* sock; ///< ZMQ socket pointer is assigned in string thread,but can be sued otherwise
  
};


struct Proxy_Thread_args : public Thread_args{

  zmq::socket_t* from_sock;
  zmq::socket_t* to_sock;
  zmq::socket_t* control;
  bool started;
  zmq::pollitem_t items[2];

};

/**
 * \class DAQUtilities
 *
 * This class can be instansiated in a Tool and provides some helpful threading, dynamic socket descovery and promotion functionality
 *
 *
 * $Author: B.Richards $
 * $Date: 2019/05/26 18:34:00 $
 * Contact: b.richards@qmul.ac.uk
 *
 */

class DAQUtilities: public Utilities{
  
 public:

  using Utilities::CreateThread;  
  DAQUtilities(zmq::context_t* zmqcontext); ///< Simple constructor
  bool AddService(std::string ServiceName, unsigned int port, bool StatusQuery=false); ///< Broadcasts an available service (only in remote mode)
  bool RemoveService(std::string ServiceName); ///< Removes service broadcasts for a service
  int UpdateConnections(std::string ServiceName, zmq::socket_t* sock, std::map<std::string,Store*> &connections, std::string port=""); ///< Dynamically connects a socket tp services broadcast with a specific name
  DAQThread_args* CreateThread(std::string ThreadName,  void (*func)(std::string));  //func = &my_int_func; ///< Create a simple thread that has string exchange with main thread
  bool MessageThread(DAQThread_args* args, std::string Message, bool block=true); ///< Send simple string to String thread
  bool MessageThread(std::string ThreadName, std::string Message, bool block=true); ///< Send simple string to String thread
  Thread_args* ZMQProxy(std::string name, zmq::socket_t* sock_1, zmq::socket_t* sock_2);
  bool KillZMQProxy(Thread_args* args);
  
  template <typename T>  bool SendPointer(zmq::socket_t* sock, T* pointer){
    
    std::stringstream tmp;
    tmp<<pointer;

    zmq::message_t message(tmp.str().length()+1);
    snprintf ((char *) message.data(), tmp.str().length()+1 , "%s" , tmp.str().c_str()) ;

   return sock->send(message);  

  } ///< Send a pointer over a ZMQ socket
  
  template<typename T> bool ReceivePointer(zmq::socket_t* sock, T*& pointer){  
   
    zmq::message_t message;
    
    if(sock->recv(&message)){   
      
      std::istringstream iss(static_cast<char*>(message.data()));
      
      //      long long unsigned int tmpP;
      unsigned long tmpP;
      iss>>std::hex>>tmpP;
      
      pointer=reinterpret_cast<T*>(tmpP);
      
      return true;
    } 
    
    else { 
      pointer=0;
      return false;
    }
    
  } ///< Receive a pointer over a ZMQ socket

     
    
    
 private:
  
  zmq::context_t *context; ///< ZMQ context pointer
  static void* String_Thread(void *arg); ///< Simpe string thread
  static void Proxy_Thread(Thread_args *arg); ///< ZMQ proxy thread 
  
};
  
  
#endif 
