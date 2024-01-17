#ifndef Logger_H
#define Logger_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "DataModel.h"

/**
 * \class Logger
 *
 * This is a demonstration tool that can receive Logging messages from ToolChains
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class Logger: public Tool {


 public:

  Logger(); ///< Construtor
  bool Initialise(std::string configfile,DataModel &data); ///< Sets up a ZMQ_PULL socket which on the log part form the config file. 
  bool Execute(); ///< Listens on socket for logging message printing them to screen when received
  bool Finalise(); ///< Closes socket and cleans up.


 private:

  int m_log_port;
  zmq::socket_t *LogReceiver; 

};


#endif
