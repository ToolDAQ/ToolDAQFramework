#ifndef DAQLOGGING_H
#define DAQLOGGING_H

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <ctime>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <functional>

#include <zmq.hpp>

#include <pthread.h>
#include <time.h>

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/date_time/posix_time/posix_time.hpp>

#include "Store.h"
#include "Logging.h"

namespace ToolFramework{
  
  /**
   * \struct DAQLogging_thread_args
   *
   *This struct holds the initalisation variables to be passed to the logging thread.
   *
   *
   * $Author: B.Richards $ 
   * $Date: 2019/05/27 18:34:00 $ 
   */
  
  
  struct DAQLogging_thread_args{
    
    DAQLogging_thread_args( zmq::context_t *incontext,  boost::uuids::uuid inUUID, std::string inlogaddress, int inlogport, std::string service){
      context=incontext;
      log_address=inlogaddress;
      UUID=inUUID;
      log_port=inlogport;
      m_service=service;
    } ///< Simple constructor to assign thread variables
    
    zmq::context_t *context; ///< pointer to ZMQ context for socket creation
    boost::uuids::uuid UUID; ///< ToolChain UUID for unique labelling of log messages
    //std::string remoteservice; 
    std::string log_address; ///< Remote multicast address 
    int log_port; ///< Port to connect to to send remote logging information
    std::string m_service;
  };
  
  
  /**
   * \class DAQLogging
   *
   *This class handels the logging, which can be directed to screen or file or over the via the ToolChain Config file
   *
   *
   * $Author: B.Richards $
   * $Date: 2019/05/27 18:34:00 $
   */
  
  
  class DAQLogging: virtual public std::ostream, public Logging {
    
  public:
    
    class TDAQStreamBuf: virtual public std::stringbuf, public Logging::TFStreamBuf{
    
  public:
    
    
    TDAQStreamBuf(zmq::context_t *context, boost::uuids::uuid UUID, std::string service, bool interactive=true, bool local=false,  std::string localpath="./log", bool remote=false, std::string log_address="239.192.1.1", int log_port=5001, bool error=false, std::ostream* filestream=0);
    
    virtual ~TDAQStreamBuf();
    
    int sync ( );
      std::function<bool(const std::string&, unsigned int, const std::string&, const unsigned int)> SendLog;
      //bool (*SendLog)(const std::string&, unsigned int, const std::string&, const unsigned int);    
    
  private:
    
    static  void *RemoteThread(void* arg);
    
    zmq::context_t *m_context;
    zmq::socket_t *LogSender;
    pthread_t thread;
    DAQLogging_thread_args *args;
    
    std::string m_service;
    bool m_remote;
    
  }; 
  
  public:
  
  /**
     Constructor for DAQLogging class
     
     @param str
     @param context Pointer to ZMW context used for creating sockets
     @param UUID ToolChain UUID for unique labelling of log messages
     @param service 
     @param mode
     @param localpath Local path for log output file 
     @param logservice Remote service to connect to to send logs
     @param logport remothe port to send logging information to
  */
  
  
  DAQLogging(zmq::context_t *context,  boost::uuids::uuid UUID, std::string service, bool interactive=true, bool local=false, std::string localpath="./log",  bool remote=false, std::string logservice="", int logport=0, bool split_output_files=false);
  
  
    virtual ~DAQLogging();
    //void SetSendLog( bool (*funptr)(const std::string&, unsigned int, const std::string&, const unsigned int));
    void SetSendLog(std::function<bool(const std::string&, unsigned int, const std::string&, const unsigned int)> funptr);
    
  
  
  private:
  
  
  };
  
}
  
#endif
  
