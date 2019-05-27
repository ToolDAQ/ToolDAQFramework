#ifndef LOGGING_H
#define LOGGING_H

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <ctime>
#include <map>

#include <zmq.hpp>

#include <pthread.h>
#include <time.h>

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/date_time/posix_time/posix_time.hpp>

#include "Store.h"

/**
 * \struct Logging_thread_args
 *
 *This struct holds the initalisation variables to be passed to the loggig thread.
 *
 *
 * $Author: B.Richards $ 
 * $Date: 2019/05/27 18:34:00 $ 
 * Contact: b.richards@qmul.ac.uk
 */
 

struct Logging_thread_args{

  Logging_thread_args( zmq::context_t *incontext,  boost::uuids::uuid inUUID, std::string inlogservice, int inlogport){
    context=incontext;
    logservice=inlogservice;
    UUID=inUUID;
    logport=inlogport;
  } ///< Simplue constructor to assing thread variables

  zmq::context_t *context; ///< pointer to ZMQ context for socket creation
  boost::uuids::uuid UUID; ///< ToolChain UUID for unique labelling of log messages
  //std::string remoteservice; 
  std::string logservice; ///< Remote service name to connect to 
  int logport; ///< Port to connect to to send remote logging information
};


/**
 * \class Logging
 *
 *This class handels the logging, which can be directed to screen or file or over the via the ToolChain Config file
 *
 *
 * $Author: B.Richards $
 * $Date: 2019/05/27 18:34:00 $
 * Contact: b.richards@qmul.ac.uk
 */


class Logging: public std::ostream {

  class MyStreamBuf: public std::stringbuf
    {

      std::ostream&   output;

    public:
      MyStreamBuf(std::ostream& str ,zmq::context_t *context,  boost::uuids::uuid UUID, std::string service, std::string mode, std::string localpath="", std::string logservice="", int logport=0);

      virtual int sync ( );

      bool ChangeOutFile(std::string localpath);

      int m_messagelevel;
      int m_verbose;

      /*
      {
	output << "[blah]" << str();
	str("");
	output.flush();
	return 0;
      }
      */
      ~MyStreamBuf();

    private:

      zmq::context_t *m_context;
      zmq::socket_t *LogSender;
      pthread_t thread;
      Logging_thread_args *args;

      std::string m_service;
      std::string m_mode;

      std::ofstream file;
      std::streambuf *psbuf, *backup;

      static  void *RemoteThread(void* arg);
     
    }; 

 public:
  MyStreamBuf buffer;

 Logging(std::ostream& str,zmq::context_t *context,  boost::uuids::uuid UUID, std::string service, std::string mode, std::string localpath="", std::string logservice="", int logport=0):std::ostream(&buffer),buffer(str, context, UUID, service, mode, localpath, logservice, logport){};

  
  //  void Log(std::string message, int messagelevel=1, int verbose=1);
  //  void Log(std::ostringstream& ost, int messagelevel=1, int verbose=1);

  template <typename T>  void Log(T message, int messagelevel=1, int verbose=1){
    std::stringstream tmp;
    tmp<<message;
    buffer.m_messagelevel=messagelevel;
    buffer.m_verbose=verbose;
    std::cout<<tmp.str()<<std::endl;
    buffer.m_messagelevel=1;
    buffer.m_verbose=1;
  } ///< Function to create a log message. Variables are:
  ///< message = templated log message text.
  ///< message level = priority level of the message being sent (e.g. if messagelevel>= verbose then message is sent).
  ///< verbose = verbosity level of the current Tool. 

  bool ChangeOutFile(std::string localpath){return buffer.ChangeOutFile(localpath);} ///< Function to change the logs out file if set to a local path. Variables are:
  ///< localpath = path to new log file

  


  /* 
  template<typename T>Logging& operator<<(T obj){ 
    std::stringstream tmp;
    tmp<<obj;
    Log(tmp.str());

    return *this;
  }
 
  template<typename T> std::ostringstream&  operator<<(T obj){
     
     oss<<obj;
     Log(oss);
     return oss;
     
     
  } 

*/
  
 private:
  

 //  static  void *LocalThread(void* arg);
 // static  void *RemoteThread(void* arg);

  //  zmq::context_t *m_context;
  // zmq::socket_t *LogSender;
  //pthread_t thread;
  
  // std::string m_mode;

  // std::ostringstream oss;
  //std::ostringstream messagebuffer; 
};

/*
Logging& endl(Logging& L){
  L << "\n";
  return L;
}


Box operator+(const Box& b)
std::string operator<<endl(){

  return "\n";
}
*/

#endif
