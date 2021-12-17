#ifndef UTILITIES_H
#define UTILITIES_H

#include <iostream>
#include <zmq.hpp>
#include <sstream>
#include <pthread.h>
#include <map>
#include <Store.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

/**
 * \struct DataModelThread_args
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

struct Thread_args{

  Thread_args(){ ///< Simple constructor 
    kill=false;
  }

  Thread_args(zmq::context_t* contextin, std::string threadname,  void (*funcin)(std::string)){ ///< Construtor for thread with string
   
    context=contextin;
    ThreadName=threadname;
    func_with_string=funcin;
    kill=false; 
  }

  Thread_args(zmq::context_t* contextin, std::string threadname,  void (*funcin)(Thread_args*)){ ///< Constrcutor for thread with args
    
    context=contextin;
    ThreadName=threadname;
    func=funcin;
    kill=false;
  }      

  virtual ~Thread_args(){ ///< virtual constructor 
    running =false;
    kill=true;
    delete sock;
    sock=0;
  }

  zmq::context_t *context; ///< ZMQ context used for ZMQ socket creation
  std::string ThreadName; ///< name of thread (deffined at creation)
  void (*func_with_string)(std::string); ///< function pointer to string thread
  void (*func)(Thread_args*); ///< function pointer to thread with args
  pthread_t thread; ///< Simple constructor underlying thread that interface is built ontop of
  zmq::socket_t* sock; ///< ZMQ socket pointer is assigned in string thread,but can be sued otherwise
  bool running; ///< Bool flag to tell the thread to run (if not set thread goes into wait cycle
  bool kill; ///< Bool flay used to kill the thread
  
};


/**
 * \class Utilities
 *
 * This class can be instansiated in a Tool and provides some helpful threading, dynamic socket descovery and promotion functionality
 *
 *
 * $Author: B.Richards $
 * $Date: 2019/05/26 18:34:00 $
 * Contact: b.richards@qmul.ac.uk
 *
 */

class Utilities{
  
 public:
  
  Utilities(zmq::context_t* zmqcontext); ///< Simple constructor
  bool AddService(std::string ServiceName, unsigned int port, bool StatusQuery=false); ///< Broadcasts an available service (only in remote mode)
  bool RemoveService(std::string ServiceName); ///< Removes service broadcasts for a service
  int UpdateConnections(std::string ServiceName, zmq::socket_t* sock, std::map<std::string,Store*> &connections, std::string port=""); ///< Dynamically connects a socket tp services broadcast with a specific name 
  Thread_args* CreateThread(std::string ThreadName,  void (*func)(std::string));  //func = &my_int_func; ///< Create a simple thread that has string exchange with main thread
  Thread_args* CreateThread(std::string ThreadName,  void (*func)(Thread_args*), Thread_args* args); ///< Create a thread with more complicated data exchange definned by arguments
  bool MessageThread(Thread_args* args, std::string Message, bool block=true); ///< Send simple string to String thread
  bool MessageThread(std::string ThreadName, std::string Message, bool block=true); ///< Send simple string to String thread
  bool KillThread(Thread_args* &args); ///< Kill a thread assosiated to args
  bool KillThread(std::string ThreadName); ///< Kill a thread by name

  template <typename T>  bool KillThread(T* pointer){ 
    
    Thread_args* tmp=pointer;
    return KillThread(tmp);

  } ///< Kill a thread with args that inheirt form base Thread_args

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
  static void* Thread(void *arg); ///< Thread with args
  std::map<std::string, Thread_args*> Threads; ///< Map of threads managed by the utilities class.
  
  
};
  
  
#endif 
