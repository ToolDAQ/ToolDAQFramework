#include "Logger.h"

Logger::Logger():Tool(){}


bool Logger::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;
  m_log= m_data->Log;

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

  m_variables.Get("log_port",m_log_port);
  
  LogReceiver = new zmq::socket_t(*m_data->context, ZMQ_PULL);
  
 
  std::stringstream tmp;
  tmp<<"tcp://*:"<<m_log_port;
  std::cout<<"connection address = "<<tmp.str()<<std::endl;
  //LogReceiver->bind(tmp.str().c_str());
  //  LogReceiver->setsockopt(ZMQ_SUBSCRIBE,"");
  LogReceiver->bind(tmp.str().c_str());
  //  printf("connection made\n");  

  return true;
}


bool Logger::Execute(){

  zmq::pollitem_t items [] = {
    { *(LogReceiver), 0, ZMQ_POLLIN, 0 }
  };


  zmq::poll (&items[0], 1, 1000);
        
  // printf("finnished poll \n");

  if (items[0].revents & ZMQ_POLLIN) {
    //   printf("in receive if \n");   
    zmq::message_t Rmessage;
    if(  LogReceiver->recv (&Rmessage)){
      // printf("got a message \n");
      std::istringstream ss(static_cast<char*>(Rmessage.data()));
      
      std::cout<<ss.str()<<std::flush;
      
      Store bb;
      
      bb.JsonParser(ss.str());
      
      std::cout<<*(bb["msg_value"])<<std::flush;
    }
  }
  
  return true;
}


bool Logger::Finalise(){

  std::stringstream tmp;
  tmp<<"tcp://*:"<<m_log_port;
  LogReceiver->disconnect(tmp.str().c_str());
  delete LogReceiver;
  LogReceiver=0;

  return true;
}
