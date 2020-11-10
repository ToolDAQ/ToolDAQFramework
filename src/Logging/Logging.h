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

#define red "\033[38;5;196m"
#define darkred "\033[38;5;88m"
#define green "\033[38;5;46m"
#define darkgreen "\033[38;5;22m"
#define blue "\033[38;5;21m"
#define darkblue "\033[38;5;18m"
#define yellow "\033[38;5;226m"
#define darkyellow "\033[38;5;142m"
#define orange "\033[38;5;208m"
#define darkorange "\033[38;5;130m"
#define pink "\033[38;5;201m"
#define darkpink "\033[38;5;129m"
#define purple "\033[38;5;57m"
#define darkpurple "\033[38;5;54m"
#define cyan  "\033[38;5;51m"
#define darkcyan  "\033[38;5;39m" 
#define white "\033[38;5;255m"
#define gray "\033[38;5;243m"
#define reset "\033[0m"


/**
 * \struct Logging_thread_args
 *
 *This struct holds the initalisation variables to be passed to the logging thread.
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
  } ///< Simple constructor to assign thread variables

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
  
  MyStreamBuf buffer; ///< Stream buffer used to replace std::cout for redirection to coustom output.
 
  /**
Constructor for Logging class

@param str
@param context Pointer to ZMW context used for creating sockets
@param UUID ToolChain UUID for unique labelling of log messages
@param service 
@param mode
@param localpath Local path for log output file 
@param logservice Remote service to connect to to send logs
@param logport remothe port to send logging information to
   */
 Logging(std::ostream& str,zmq::context_t *context,  boost::uuids::uuid UUID, std::string service, std::string mode, std::string localpath="", std::string logservice="", int logport=0):std::ostream(&buffer),buffer(str, context, UUID, service, mode, localpath, logservice, logport){};

  
  //  void Log(std::string message, int messagelevel=1, int verbose=1);
  //  void Log(std::ostringstream& ost, int messagelevel=1, int verbose=1);

  /**
       Function to create a log messages. 
 
       @param message templated log message text.
       @param messagelevel message verbosity level of the message being sent (e.g. if 'messagelevel>= verbose' Then message is sent). 
       @param verbose verbosity level of the current Tool.    
       
  */
  template <typename T>  void Log(T message, int messagelevel=1, int verbose=1){
    if(messagelevel<=verbose){    
      std::stringstream tmp;
      tmp<<message;
      buffer.m_messagelevel=messagelevel;
      buffer.m_verbose=verbose;
      std::cout<<tmp.str()<<std::endl;
      buffer.m_messagelevel=1;
      buffer.m_verbose=1;
    } 
  }


  /**
     Functionn to change the logs out file if set to a local path.

     @param localpath path to new log file.
     @return value is bool success of opening new logfile.
 
 */
  bool ChangeOutFile(std::string localpath){return buffer.ChangeOutFile(localpath);} 

  


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
