#ifndef MYTOOLServiceAdd_H
#define MYTOOLServiceAdd_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "DataModel.h"

/**
 * \class MyToolServiceAdd
 *
 * This is a template for a Tool to publish a service via ToolDAQ dynamic service discovery. Please fill out the descripton and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
*/
class MyToolServiceAdd: public Tool {


 public:

  MyToolServiceAdd(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Executre function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise funciton used to clean up resorces.


 private:

  DAQUtilities* m_util;  ///< Pointer to utilities class to help with threading
  zmq::socket_t* sock;  ///< zmq socket pointer for socket to advertise
  int m_port;  ///< Port to advertise

};


#endif
