#include "ToolDAQChain.h"

ToolDAQChain::ToolDAQChain(std::string configfile,  int argc, char* argv[]){
  m_log_mode="";

  if(!m_data.vars.Initialise(configfile)){
    std::cout<<"\033[38;5;196m ERROR!!!: No valid config file quitting \033[0m"<<std::endl;
    exit(1);
  }

  if(!m_data.vars.Get("verbose",m_verbose)) m_verbose=9;
  if(!m_data.vars.Get("error_level",m_errorlevel)) m_errorlevel=2;
  if(!m_data.vars.Get("remote_port",m_remoteport)) m_remoteport=24004; 
  if(!m_data.vars.Get("attempt_recover",m_recover)) m_recover=false;
  if(!m_data.vars.Get("log_mode",m_log_mode)) m_log_mode="Interactive";
  if(!m_data.vars.Get("log_local_path",m_log_local_path))  m_log_local_path="./log";
  if(!m_data.vars.Get("service_discovery_address",m_multicastaddress)) m_multicastaddress="239.192.1.1";
  if(!m_data.vars.Get("service_discovery_port",m_multicastport)) m_multicastport=5000;
  if(!m_data.vars.Get("service_name",m_service)) m_service="ToolDAQ_Service";
  if(!m_data.vars.Get("log_service",m_log_service)) m_log_service="LogStore";
  if(!m_data.vars.Get("log_port",m_log_port)) m_log_port=24010;
  if(!m_data.vars.Get("service_publish_sec",m_pub_sec)) m_pub_sec=5;
  if(!m_data.vars.Get("service_kick_sec",m_kick_sec)) m_kick_sec=60;
  
  if(!m_data.vars.Get("Inline", m_inline))  m_inline=0;
  if(!m_data.vars.Get("Interactive", m_interactive)) m_interactive=false;
  if(!m_data.vars.Get("Remote", m_remote)) m_remote=false;

  
  unsigned int IO_Threads=1;
  m_data.vars.Get("IO_Threads",IO_Threads);
  
  m_data.vars.Set("argc",argc);
  for(int i=0; i<argc ; i++){
    
    std::stringstream tmp;
    tmp<<"$"<<i;
    m_data.vars.Set(tmp.str(),argv[i]);
    
  }
 
  Init(IO_Threads);
  
  std::string toolsfile="";
  m_data.vars.Get("Tools_File",toolsfile);
  
  if(!LoadTools(toolsfile)) exit(1);
  
  if(m_inline!=0) Inline();  
  else if(m_interactive) Interactive(); 
  else if(m_remote) Remote();
  
  
}

ToolDAQChain::ToolDAQChain(int verbose, int errorlevel, std::string service, std::string logmode,std::string log_local_path, std::string log_service, int log_port, int pub_sec, int kick_sec,unsigned int IO_Threads){
  
  m_verbose=verbose;
  m_errorlevel=errorlevel;
  m_log_mode = logmode;
  m_log_local_path=log_local_path;
  m_service=service;
  m_log_service=log_service;
  m_log_port=log_port;
  m_pub_sec=pub_sec;
  m_kick_sec=kick_sec;
  
  Init(IO_Threads);
  
  
}

void ToolDAQChain::Init(unsigned int IO_Threads){
 
  context=new zmq::context_t(IO_Threads);
  m_data.context=context;
  
  m_UUID = boost::uuids::random_generator()();
  
  if(m_log_mode!="Off"){
    std::cout<<"in off if"<<std::endl;    
    bcout=std::cout.rdbuf();
    out=new  std::ostream(bcout);
  }

  m_data.Log= new DAQLogging(*out, context, m_UUID, m_service, m_log_mode, m_log_local_path, m_log_service, m_log_port);
  
  if(m_log_mode!="Off") std::cout.rdbuf((m_data.Log->buffer));
  
  execounter=0;
  Initialised=false;
  Finalised=true;
  paused=false;
  
  msg_id=0;
  
  bool sendflag=true;
  bool receiveflag=true;
  
  if(m_pub_sec<0 || m_inline>0 || m_interactive) sendflag=false;
  if(m_kick_sec<0) receiveflag=false;

  SD=new ServiceDiscovery(sendflag,receiveflag, m_remoteport, m_multicastaddress.c_str(),m_multicastport,context,m_UUID,m_service,m_pub_sec,m_kick_sec);
  
  if(m_log_mode=="Remote") sleep(10); //needed to allow service discovery find time
  
  *m_log<<MsgL(1,m_verbose)<<cyan<<"UUID = "<<m_UUID<<std::endl<<yellow<<"********************************************************\n"<<"**** Tool chain created ****\n"<<"********************************************************"<<std::endl;

}





void ToolDAQChain::Remote(int portnum, std::string SD_address, int SD_port){
  
  m_remoteport=portnum;
  m_multicastport=SD_port;
  m_multicastaddress=SD_address;
  
  bool sendflag=true;
  bool receiveflag=true;
  
  if(m_pub_sec<0) sendflag=false;
  if(m_kick_sec<0) receiveflag=false;
  
  SD=new ServiceDiscovery(sendflag,receiveflag, m_remoteport, m_multicastaddress.c_str(),m_multicastport,context,m_UUID,m_service,m_pub_sec,m_kick_sec);
  
  Remote();
  
}

void ToolDAQChain::Remote(){
  
  //m_verbose=false;
  exeloop=false;

  std::stringstream tcp;
  tcp<<"tcp://*:"<<m_remoteport;

  zmq::socket_t Ireceiver (*context, ZMQ_REP);
  //int a=120000;
  //Ireceiver.setsockopt(ZMQ_RCVTIMEO, a);
  //Ireceiver.setsockopt(ZMQ_SNDTIMEO, a);
  
  Ireceiver.bind(tcp.str().c_str());
  
  zmq::pollitem_t out[]={{Ireceiver,0,ZMQ_POLLOUT,0}};

  bool running=true;
  
  while (running){

    zmq::message_t message;
    std::string command="";
    if(Ireceiver.recv(&message, ZMQ_NOBLOCK)){
      
      std::istringstream iss(static_cast<char*>(message.data()));
      command=iss.str();

      Store rr;
      rr.JsonParser(command);
      if(*rr["msg_type"]=="Command") command=*rr["msg_value"];
      else command="NULL";


       boost::posix_time::ptime t = boost::posix_time::microsec_clock::universal_time();
      std::stringstream isot;
      isot<<boost::posix_time::to_iso_extended_string(t) << "Z";

      msg_id++;
      Store bb;

      bb.Set("uuid",m_UUID);
      bb.Set("msg_id",msg_id);
      *bb["msg_time"]=isot.str();
      *bb["msg_type"]="Command Reply";
      bb.Set("msg_value",ExecuteCommand(command));
    

      std::string tmp="";
      bb>>tmp;
      zmq::message_t send(tmp.length()+1);
      snprintf ((char *) send.data(), tmp.length()+1 , "%s" ,tmp.c_str()) ;
    
      zmq::poll(out,1,2000);
      if (out[0].revents & ZMQ_POLLOUT)  Ireceiver.send(send);
      
      // std::cout<<"received "<<command<<std::endl;
      //std::cout<<"sent "<<tmp<<std::endl;
      if(command=="Quit"){
	//printf("%s /n","receved quit");
	running=false;
	/*
	  zmq::socket_t ServicePublisher (*context, ZMQ_PUSH);
	  ServicePublisher.connect("inproc://ServicePublish");
	  zmq::socket_t ServiceDiscovery (*context, ZMQ_DEALER);	
	  ServiceDiscovery.connect("inproc://ServiceDiscovery");
	  
	  zmq::message_t Qsignal1(256);
	  zmq::message_t Qsignal2(256);
	  std::string tmp="Quit";
	  snprintf ((char *) Qsignal1.data(), 256 , "%s" ,tmp.c_str()) ;
	  snprintf ((char *) Qsignal2.data(), 256 , "%s" ,tmp.c_str()) ;
	  ServicePublisher.send(Qsignal1);
	  ServiceDiscovery.send(Qsignal2);
	  //exit(0);
	  */
      }

      command="";
    }
  
    ExecuteCommand(command);
   
  }  
  //printf("%s /n","finnished remote");
  
}





ToolDAQChain::~ToolDAQChain(){
  
  for (int i=0;i<m_tools.size();i++){
    delete m_tools.at(i);
    m_tools.at(i)=0;
  }
  
  m_tools.clear();
  
  //  printf("%s \n","tdebug 1");
  delete m_data.Log;  
  //printf("%s \n","tdebug 2");
  m_data.Log=0;
  //printf("%s \n","tdebug 3");  
  delete SD;
  //printf("%s \n","tdebug 4");
  SD=0;  
  //printf("%s \n","tdebug 5");
  //sleep(30);
  //printf("%s \n","tdebug 6");
  //  context->close();
  //printf("%s \n","tdebug 6.5");
  //delete context;
  // printf("%s \n","tdebug 7");
  context=0;
  //printf("%s \n","tdebug 8");
  if(m_log_mode!="Off"){
    std::cout.rdbuf(bcout);
    //printf("%s \n","tdebug 9");
    delete out;
    //printf("%s \n","tdebug 10");
    out=0;
    //printf("%s \n","tdebug 11");
  }  
  
  
  
}

