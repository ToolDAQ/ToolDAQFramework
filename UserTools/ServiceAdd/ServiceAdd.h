#ifndef ServiceAdd_H
#define ServiceAdd_H

#include <string>
#include <iostream>

#include "Tool.h"

#include <boost/uuid/uuid.hpp>            // uuid class                                                                     
#include <boost/uuid/uuid_generators.hpp> // generators                                                                     
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.        


/**
 * \class ServiceAdd
 *
 * This Tools demonstraits how to add services and publicise connections for them in the ToolCahins multicast beacons. 
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/

class ServiceAdd: public Tool {


 public:

  ServiceAdd(); ///< Constrcutor
  bool Initialise(std::string configfile,DataModel &data); ///< Opens socket to service discoverypublisher and adds a service and assosiated port to broadcast. 
  bool Execute(); ///< Does nothing
  bool Finalise(); ///< Opens new connection to service discovery publish thread and removes service from service list.


 private:





};


#endif
