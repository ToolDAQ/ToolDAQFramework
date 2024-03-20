#ifndef TOOLDAQCHAIN_H
#define TOOLDAQCHAIN_H

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <pthread.h>
#include <time.h> 
#include <sys/stat.h>

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/date_time/posix_time/posix_time.hpp>
//#include <boost/progress.hpp>

#include "Tool.h"
#include "ToolChain.h"
#include "DAQDataModelBase.h"
#include "DAQLogging.h"
#include "Logging.h"
#include "zmq.hpp"
#include "Factory.h"
#include "Store.h"
#include "ServiceDiscovery.h"

class DataModel;

namespace ToolFramework{
  
  /**
   * \struct ToolDAQChainargs
   *
   * Simple struct to pass thread initalisation variables to Interactive ToolDAQChain thread
   *
   * $Author: B.Richards $
   * $Date: 2019/05/28 10:44:00 $
   * Contact: b.richards@qmul.ac.uk
   */
  struct ToolDAQChainargs{
    
    ToolDAQChainargs(){};
    
    zmq::context_t* context; ///< ZMQ context used for creating sockets.
    bool *msgflag; ///< Message flag used to indiacte if a new interactive command has been submitted to the interactive thread and needs execution on the main thread. 
    
  };
  
  /**
   * \class ToolDAQChain
   *
   * This class holds a dynamic list of Tools which can be Initialised, Executed and Finalised to perform program operation. Large number of options in terms of run modes and setting can be assigned.
   *
   * $Author: B.Richards $
   * $Date: 2019/05/28 10:44:00 $
   * Contact: b.richards@qmul.ac.uk
   */
  class ToolDAQChain : public ToolChain {
    
  public:
    ToolDAQChain(std::string configfile, DataModel* data_model, int argc=0, char* argv[]=0); ///< Constructor that obtains all of the configuration varaibles from an input file. @param configfile The path and name of the config file to read configuration values from.
    
    /**
       Constructor with explicit configuration variables passed as arguments.
       @param verbose The verbosity level of the ToolDAQChain. The higher the number the more explicit the print outs. 0 = silent apart from errors.
       @param errorlevel The behavior that occurs when the ToolDAQChain encounters an error. 0 = do not exit for both handeled and unhandeled errors, 1 = exit on unhandeled errors only, 2 = exit on handeled and unhandeled errors.
       @param service The name of the ToolDAQChain service to boradcast for service discovery
       @param logmode Where log printouts should be forwarded too. "Interactive" = cout, "Remote" = send to a remote logging system, "local" = ouput logs to a file. 
       @param log_local_path The file path and name of where to store logs if in Local logging mode.
       @param log_service Service name of remote logging system if in Remote logging mode.
       @param log_port The Port number of the remote logging service
       @param pub_sec The number of seconds between publishing multicast serivce beacons
       @param kick_sec The number of seconds to wait before removing a servic from the remote services list if no beacon received.
       @param IO_Threads The number of ZMQ IO threads to use (~1 per Gbps of traffic).
    */
    ToolDAQChain(int verbose, int errorlevel=0, std::string service="test",  bool interactive=true, bool local=false, std::string log_local_path="./log", bool remote=false,  std::string log_service="", int log_port=0, bool split_output_files=false, int pub_sec=5, int kick_sec=60, unsigned int IO_Threads=1, DataModel* in_data_model=0); 
    //verbosity: true= print out status messages , false= print only error messages;
    //errorlevels: 0= do not exit; error 1= exit if unhandeled error ; exit 2= exit on handeled and unhandeled errors; 
    ~ToolDAQChain(); 
    void Remote(int portnum, std::string SD_address="239.192.1.1", int SD_port=5000); ///< Run ToolChain in remote mode, where connands are received fomr network connections. @param portnum The port number to listen for remote connections on. @param SD_address The service discovery address to publish availability beacons for remote connections on. @param SD_port the multicast port to used for service discovery beacons
    
  private:
    
    void Init(unsigned int IO_Threads);
    void Init(){};
    void Remote();
    
    ServiceDiscovery *SD;

    DAQDataModelBase* m_DAQdata;
    
    //conf variables
    boost::uuids::uuid m_UUID;
    std::string m_UUID_path;
    
    bool m_log_remote; 
    std::string m_log_service;
    int m_log_port;
    std::string m_service;
    bool m_remote;
    
    //socket coms and threading variables
    pthread_t DAQthread[2];
    zmq::context_t *context;
    int m_remoteport;
    int m_multicastport;
    std::string m_multicastaddress;
    long msg_id;
    int m_pub_sec;
    int m_kick_sec;

    
  };

}

#endif
