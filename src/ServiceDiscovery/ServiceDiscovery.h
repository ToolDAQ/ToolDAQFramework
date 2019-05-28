#ifndef SERVICEDISCOVERY_H
#define SERVICEDISCOVERY_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/date_time/posix_time/posix_time.hpp>

#include "zmq.hpp"

#include "Store.h"

/**
 * \struct thread_args
 *
 *This struct holds the initalisation variables to be passed to the ServiceDiscovery thread.
 *
 *
 * $Author: B.Richards $
 * $Date: 2019/05/27 18:34:00 $
 * Contact: b.richards@qmul.ac.uk                                                                                                                                                                     */

struct thread_args{
  
  thread_args(boost::uuids::uuid inUUID, zmq::context_t *incontext,std::string inmulticastaddress, int inmulticastport, std::string inservice, int inremoteport, int  inpubsec=5, int inkicksec=5){
    UUID=inUUID;
    context=incontext;
    multicastaddress=inmulticastaddress;
    multicastport=inmulticastport;
    service=inservice;
    remoteport=inremoteport;
    pubsec=inpubsec;
    kicksec=inkicksec;
  } ///< Simple construtor to assign thread variables
  
  boost::uuids::uuid UUID; ///< Unique UUID identifier for the ToolChain to be used is service beacons.
  zmq::context_t *context; ///< ZMQ context pointer requred for creating sockets
  std::string multicastaddress; ///< Multicast address to send beacons
  int multicastport; ///< Port to use for multicast beacons
  std::string service; ///< Name to broadcast to identify the ToolChain type
  int remoteport; ///< Port for remote connections to control the ToolChain
  int pubsec; ///< The number of seconds between sending multicast beacons
  int kicksec; ///< The number of seconds without receiving a beacon from a remote service to remove them from the services list.
};


/**
 * \class ServiceDiscovery
 *
 * This class handels the service discovery for the ToolChain. It oth handels publishing multicast beacons for the ToolCahin and any other customs services and it collates and maintains a list of other remote services form their multicast beacons that Tools can acess via a ZMQ port and query.
 *
 * $Author: B.Richards $
 * $Date: 2019/05/27 18:34:00 $
 * Contact: b.richards@qmul.ac.uk
 */

class ServiceDiscovery{
    
 public:

  /**
     Constructor for SErviceDiscovery
     
     @param Send bool to determin if the publish thread is created to send multicast beacons
     @param Receive boot to determin if the listen thread is created to receive and collate incomming multicast beacons 
     @param remoteport Port number to publish for remote connections to control the ToolChain
     @param address 
     @param multicastport Port to use for multicast beacons
     @param incontext Pointer to  ZMQ contect for socket creation
     @param UUID Unique UUID identifier for the ToolChain to be used is service beacons
     @param service Name to broadcast to identify the ToolChain type in beacon
     @param pubsec The number of seconds between sending multicast beacons 
     @param kicksec The number of seconds without receiving a beacon from a remote service to remove them from the services list.
     
  */
  ServiceDiscovery(bool Send, bool Receive, int remoteport, std::string address, int multicastport, zmq::context_t * incontext, boost::uuids::uuid UUID, std::string service, int pubsec=5, int kicksec=60);

  /**
     Simpler constructor for only receiving multicast beacons and not sending. This therefor doesnt require the UUID and plublish variables, so can be used for passive listener.

     variables are the same as above.
  */
  ServiceDiscovery( std::string address, int multicastport, zmq::context_t * incontext, int kicksec=60);  
  ~ServiceDiscovery(); ///< Simple destructor
  
  
 private:
  
  /**
     Thread function for publishing multicast beacons
     @param arg pointer to thread argument struct
  */
  static  void *MulticastPublishThread(void* arg);
  
  /**
     Thread function for publishing multicast beacons
     @param arg pointer to thread argument struct
  */
  static  void *MulticastListenThread(void* arg);
  
  boost::uuids::uuid m_UUID; ///< Unique UUID identifier for the ToolChain to be used is service beacons.
  zmq::context_t *context; ///< ZMQ context pointer requred for creating sockets
  pthread_t thread[2]; ///< Array to hold thread identifiers
  thread_args *args; ///< Pointer to hold thread arguments
  
  int m_multicastport; ///< Port to use for multicast beacons
  std::string m_multicastaddress; ///< Multicast address to send beacons
  std::string m_service; ///< Name to broadcast to identify the ToolChain type
  int m_remoteport; ///< Port for remote connections to control the ToolChain
  
  bool m_send; ///< Send bool to determin if the publish thread is created to send multicast beacons     
  bool m_receive; ///< Receive boot to determin if the listen thread is created to receive and collate incomming multicast beacons
  
  
  
};

#endif
