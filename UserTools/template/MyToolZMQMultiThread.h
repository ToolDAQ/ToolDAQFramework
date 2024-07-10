#ifndef MYTOOLZMQMultiThread_H
#define MYTOOLZMQMultiThread_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "DataModel.h"

/**
 * \struct ZMQMyToolMultiThread_args
 *
 * This is a struct to place data you want your thread to acess or exchange with it. The idea is the datainside is only used by the threa\
d and so will be thread safe
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
*/


struct MyToolZMQMultiThread_args:DAQThread_args{

  MyToolZMQMultiThread_args();
  ~MyToolZMQMultiThread_args();

  zmq::socket_t* ThreadReceive;
  zmq::socket_t* ThreadSend;
    
  zmq::pollitem_t initems[1];
  zmq::pollitem_t outitems[1];

};

/**
 * \class MyToolZMQMultiThread
 *
 * This is a template for a Tool that provides multiple worker threads and comunicates with them via ZMQ in a thread safe way. Please fill out the descripton and author information.
 *
 * $Author: B.Richards $
 * $Date: 2019/05/28 10:44:00 $
 */
class MyToolZMQMultiThread: public Tool {


 public:

  MyToolZMQMultiThread(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Executre function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise funciton used to clean up resorces.


 private:

  static void Thread(Thread_args* arg);  ///< Function to be run by the thread in a loop. Make sure not to block in it
  DAQUtilities* m_util; ///< Pointer to utilities class to help with threading
  std::vector<MyToolZMQMultiThread_args*> args; ///< Vector of thread args (also holds pointers to the threads)

  zmq::pollitem_t items[2]; ///< This is used to both inform the poll and store its output. Allows for multitasking sockets 
  zmq::socket_t* ManagerSend; ///< Socket to send information to threads
  zmq::socket_t* ManagerReceive; ///< Socket to receive information form threads

  int m_freethreads; ///< Keeps track of free threads

};


#endif
