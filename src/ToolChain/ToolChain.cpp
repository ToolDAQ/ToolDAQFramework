#include "ToolChain.h"

ToolChain::ToolChain(std::string configfile,  int argc, char* argv[]){
  
  m_data.vars.Initialise(configfile);

  Store config;
  config.Initialise(configfile);
  config.Get("verbose",m_verbose);
  config.Get("error_level",m_errorlevel);
  config.Get("remote_port",m_remoteport);
  config.Get("attempt_recover",m_recover);
  config.Get("log_mode",m_log_mode);
  config.Get("log_local_path",m_log_local_path);
  config.Get("service_discovery_address",m_multicastaddress);
  config.Get("service_discovery_port",m_multicastport);
  config.Get("service_name",m_service);
  config.Get("log_service",m_log_service);
  config.Get("log_port",m_log_port);
  config.Get("service_publish_sec",m_pub_sec);
  config.Get("service_kick_sec",m_kick_sec);

  unsigned int IO_Threads=1;
  config.Get("IO_Threads",IO_Threads);
 
  m_data.vars.Set("argc",argc);
  for(int i=0; i<argc ; i++){

    std::stringstream tmp;
    tmp<<"$"<<i;
    m_data.vars.Set(tmp.str(),argv[i]);

  }

  
  Init(IO_Threads);

  Inline=0;
  interactive=false;
  remote=false;
  config.Get("Inline",Inline);
  config.Get("Interactive",interactive);
  config.Get("Remote",remote);

  bool sendflag=true;
  bool receiveflag=true;

  if(m_pub_sec<0 || Inline>0 || interactive) sendflag=false;
  if(m_kick_sec<0) receiveflag=false;
 
  SD=new ServiceDiscovery(sendflag,receiveflag, m_remoteport, m_multicastaddress.c_str(),m_multicastport,context,m_UUID,m_service,m_pub_sec,m_kick_sec);

  if(m_log_mode=="Remote") sleep(10); //needed to allow service discovery find time                                                                                       
  logmessage<<cyan<<"UUID = "<<m_UUID<<std::endl<<yellow<<"********************************************************"<<std::endl<<"**** Tool chain created ****"<<std::endl<<"********************************************************"<<plain<<std::endl;
  m_data.Log->Log(logmessage.str(),1,m_verbose);
  logmessage.str("");


  std::string toolsfile="";
  config.Get("Tools_File",toolsfile);

  if(toolsfile!=""){
    std::ifstream file(toolsfile.c_str());
    std::string line;
    if(file.is_open()){

      while (getline(file,line)){
        if (line.size()>0){
          if (line.at(0)=='#')continue;
	  std::string name;
	  std::string tool;
	  std::string conf;
	  std::stringstream stream(line);

          if(stream>>name>>tool>>conf) Add(name,Factory(tool),conf);

        }
      }
    }

    file.close();

  }

  if(Inline==-1){
    bool StopLoop=false;
    m_data.vars.Set("StopLoop",StopLoop);
    Initialise();
    while(!StopLoop){
      Execute();
      m_data.vars.Get("StopLoop",StopLoop);
    }
    Finalise();

  }

  if(Inline>0){
    Initialise();
    Execute(Inline);
    Finalise();
    //exit(0);
  }

  else if(interactive) Interactive();
  
  else if(remote) Remote();
  
    
  //printf("%s \n","finnished constructor");
}

ToolChain::ToolChain(int verbose, int errorlevel, std::string service, std::string logmode,std::string log_local_path, std::string log_service, int log_port, int pub_sec, int kick_sec,unsigned int IO_Threads){

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

void ToolChain::Init(unsigned int IO_Threads){

  context=new zmq::context_t(IO_Threads);
  m_data.context=context;

  m_UUID = boost::uuids::random_generator()();

  if(m_log_mode!="Off"){
 
  bcout=std::cout.rdbuf();
  out=new  std::ostream(bcout);
  }
  m_data.Log= new Logging(*out, context, m_UUID, m_service, m_log_mode, m_log_local_path, m_log_service, m_log_port);

  if(m_log_mode!="Off") std::cout.rdbuf(&(m_data.Log->buffer));
  /*
    if(m_verbose){ 
    *(m_data.Log)<<"UUID = "<<m_UUID<<std::endl;
    *(m_data.Log)<<"********************************************************"<<std::endl;
    *(m_data.Log)<<"**** Tool chain created ****"<<std::endl;
    *(m_data.Log)<<"********************************************************"<<std::endl;
    }
  */
  //  logmessage<<"UUID = "<<m_UUID<<std::endl<<"********************************************************"<<std::endl<<"**** Tool chain created ****"<<std::endl<<"********************************************************"<<std::endl;
  // m_data.Log->Log(logmessage.str(),1,m_verbose);
  //logmessage.str("");


  execounter=0;
  Initialised=false;
  Finalised=true;
  paused=false;

  msg_id=0;
}



void ToolChain::Add(std::string name,Tool *tool,std::string configfile){
  if(tool!=0){
    // if(m_verbose)*(m_data.Log)<<"Adding Tool=\""<<name<<"\" tool chain"<<std::endl;
    logmessage<<cyan<<"Adding Tool='"<<name<<"' to ToolChain"<<plain;
    m_data.Log->Log(logmessage.str(),1,m_verbose);
    logmessage.str("");
    
    m_tools.push_back(tool);
    m_toolnames.push_back(name);
    m_configfiles.push_back(configfile);
    
    //    if(m_verbose)*(m_data.Log)<<"Tool=\""<<name<<"\" added successfully"<<std::endl<<std::endl; 
    logmessage<<green<<"Tool='"<<name<<"' added successfully"<<plain<<std::endl;
    m_data.Log->Log(logmessage.str(),1,m_verbose);
    logmessage.str("");
    
  }
  else{
    logmessage<<red<<"WARNING!!! Tool='"<<name<<"' Does Not Exist in factory!!! "<<plain<<std::endl;
    m_data.Log->Log(logmessage.str(),0,m_verbose);
    logmessage.str("");
  }

}



int ToolChain::Initialise(){

  bool result=0;

  if (Finalised){
    logmessage<<yellow<<"********************************************************"<<std::endl<<"**** Initialising tools in toolchain ****"<<std::endl<<"********************************************************"<<plain<<std::endl;
    m_data.Log->Log(logmessage.str(),1,m_verbose);
    logmessage.str("");

    /*if(m_verbose){
      *(m_data.Log)<<"********************************************************"<<std::endl;
      *(m_data.Log)<<"**** Initialising tools in toolchain ****"<<std::endl;
      *(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
      }*/
    
    for(int i=0 ; i<m_tools.size();i++){  
      
      logmessage<<cyan<<"Initialising "<<m_toolnames.at(i)<<plain;
      m_data.Log->Log(logmessage.str(),2,m_verbose);
      logmessage.str("");

      //if(m_verbose) *(m_data.Log)<<"Initialising "<<m_toolnames.at(i)<<std::endl;

#ifndef DEBUG      
      try{ 
#endif   

	if(m_tools.at(i)->Initialise(m_configfiles.at(i), m_data)){
	  //  if(m_verbose)*(m_data.Log)<<m_toolnames.at(i)<<" initialised successfully"<<std::endl<<std::endl;
	  logmessage<<green<<m_toolnames.at(i)<<" initialised successfully"<<plain<<std::endl;
	  m_data.Log->Log( logmessage.str(),2,m_verbose);
	  logmessage.str("");
	}
	else{
	  //*(m_data.Log)<<"WARNING !!!!! "<<m_toolnames.at(i)<<" Failed to initialise (exit error code)"<<std::endl<<std::endl;
	  logmessage<<red<<"WARNING !!!!! "<<m_toolnames.at(i)<<" Failed to initialise (exit error code)"<<plain<<std::endl;
          m_data.Log->Log( logmessage.str(),0,m_verbose);
          logmessage.str("");
	  result=1;
	  if(m_errorlevel>1) exit(1);
         }

#ifndef DEBUG	
    }      
      catch(...){
	//*(m_data.Log)<<"WARNING !!!!! "<<m_toolnames.at(i)<<" Failed to initialise (uncaught error)"<<std::endl<<std::endl;
	logmessage<<red<<"WARNING !!!!! "<<m_toolnames.at(i)<<" Failed to initialise (uncaught error)"<<plain<<std::endl;
	m_data.Log->Log( logmessage.str(),0,m_verbose);
	logmessage.str("");
	result=2;
	if(m_errorlevel>0) exit(1);
       }
#endif
     
    }
    
    //   if(m_verbose){*(m_data.Log)<<"**** Tool chain initilised ****"<<std::endl;;
    // *(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
    //  }
    
    logmessage<<std::endl<<yellow<<"**** Tool chain initialised ****"<<std::endl<<"********************************************************"<<plain<<std::endl;
    m_data.Log->Log( logmessage.str(),1,m_verbose);
    logmessage.str("");

    execounter=0;
    Initialised=true;
    Finalised=false;
    }
  else {
    //*(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
    // *(m_data.Log)<<" ERROR: ToolChain Cannot Be Initialised as already running. Finalise old chain first";
    //*(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
    logmessage<<purple<<"********************************************************"<<std::endl<<std::endl<<" ERROR: ToolChain Cannot Be Initialised as already running. Finalise old chain first"<<"********************************************************"<<plain<<std::endl;
    m_data.Log->Log( logmessage.str(),0,m_verbose);
    logmessage.str("");


    result=-1;
  }
  
  return result;
}



int ToolChain::Execute(int repeates){
  //boost::progress_timer t;
  int result =0;
  
  if(Initialised){

    if(Inline){
      logmessage<<yellow<<"********************************************************"<<std::endl<<"**** Executing toolchain "<<repeates<<" times ****"<<std::endl<<"********************************************************"<<plain<<std::endl;
    m_data.Log->Log( logmessage.str(),2,m_verbose);
    logmessage.str("");
    }

    for(int i=0;i<repeates;i++){
      /*
	if(m_verbose){
	*(m_data.Log)<<"********************************************************"<<std::endl;
	*(m_data.Log)<<"**** Executing tools in toolchain ****"<<std::endl;
	*(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
	}
      */
      logmessage<<yellow<<"********************************************************"<<std::endl<<"**** Executing tools in toolchain ****"<<std::endl<<"********************************************************"<<plain<<std::endl;
      m_data.Log->Log( logmessage.str(),3,m_verbose);
      logmessage.str("");
      
      for(int i=0 ; i<m_tools.size();i++){
	
	//if(m_verbose)    *(m_data.Log)<<"Executing "<<m_toolnames.at(i)<<std::endl;
	logmessage<<cyan<<"Executing "<<m_toolnames.at(i)<<plain;
	m_data.Log->Log( logmessage.str(),4,m_verbose);
	logmessage.str("");	
	
#ifndef DEBUG
	try{
#endif

	  if(m_tools.at(i)->Execute()){
	    //    if(m_verbose)*(m_data.Log)<<m_toolnames.at(i)<<" executed  successfully"<<std::endl<<std::endl;
	    logmessage<<green<<m_toolnames.at(i)<<" executed  successfully"<<plain<<std::endl;
	    m_data.Log->Log( logmessage.str(),4,m_verbose);
	    logmessage.str("");
	    
	  }
	  else{
	    //*(m_data.Log)<<"WARNING !!!!!! "<<m_toolnames.at(i)<<" Failed to execute (error code)"<<std::endl<<std::endl;
	    logmessage<<red<<"WARNING !!!!!! "<<m_toolnames.at(i)<<" Failed to execute (error code)"<<plain<<std::endl;
            m_data.Log->Log( logmessage.str(),0,m_verbose);
            logmessage.str("");
	    
	    
	    result=1;
	    if(m_errorlevel>1){
	      if(m_recover){
		m_errorlevel=0;
		Finalise();
	      }
	      exit(1);
	    }
	  }  

#ifndef DEBUG
	}
	
	catch(...){
	  // *(m_data.Log)<<"WARNING !!!!!! "<<m_toolnames.at(i)<<" Failed to execute (uncaught error)"<<std::endl<<std::endl;
	  logmessage<<red<<"WARNING !!!!!! "<<m_toolnames.at(i)<<" Failed to execute (uncaught error)"<<plain<<std::endl;
	  m_data.Log->Log( logmessage.str(),0,m_verbose);
	  logmessage.str("");
	  
	  result=2;
	  if(m_errorlevel>0){
	    if(m_recover){
		m_errorlevel=0;
		Finalise();
	    }
	    exit(1);
	  }
	}
#endif	

      }
      /*      if(m_verbose){
       *(m_data.Log)<<"**** Tool chain executed ****"<<std::endl;
       *(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
       }
      */
      logmessage<<yellow<<"**** Tool chain executed ****"<<std::endl<<"********************************************************"<<plain<<std::endl;
      m_data.Log->Log( logmessage.str(),3,m_verbose);
      logmessage.str("");
    }
    
    execounter++;
    if(Inline){
      logmessage<<yellow<<"********************************************************"<<std::endl<<"**** Executed toolchain "<<repeates<<" times ****"<<std::endl<<"********************************************************"<<plain<<std::endl;
      m_data.Log->Log( logmessage.str(),2,m_verbose);
      logmessage.str("");
    }
  }
  
  else {
    /*
    *(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
    *(m_data.Log)<<" ERROR: ToolChain Cannot Be Executed As Has Not Been Initialised yet."<<std::endl;
    *(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
    */

    logmessage<<purple<<"********************************************************"<<std::endl<<std::endl<<" ERROR: ToolChain Cannot Be Executed As Has Not Been Initialised yet."<<std::endl<<"********************************************************"<<plain<<std::endl;
    m_data.Log->Log( logmessage.str(),0,m_verbose);
    logmessage.str("");
    result=-1;
  }

  return result;
}



int ToolChain::Finalise(){
 
  int result=0;

  if(Initialised){
    /*
    if(m_verbose){
      *(m_data.Log)<<"********************************************************"<<std::endl;
      *(m_data.Log)<<"**** Finalising tools in toolchain ****"<<std::endl;
      *(m_data.Log)<<"***`<*****************************************************"<<std::endl<<std::endl;
    }  
    */
    logmessage<<yellow<<"********************************************************"<<std::endl<<"**** Finalising tools in toolchain ****"<<std::endl<<"********************************************************"<<plain<<std::endl;
    m_data.Log->Log( logmessage.str(),1,m_verbose);
    logmessage.str("");

    for(int i=0 ; i<m_tools.size();i++){
      
      //if(m_verbose)*(m_data.Log)<<"Finalising "<<m_toolnames.at(i)<<std::endl;
      logmessage<<cyan<<"Finalising "<<m_toolnames.at(i)<<plain;
      m_data.Log->Log( logmessage.str(),2,m_verbose);
      logmessage.str("");

#ifndef DEBUG
      try{
#endif

	if(m_tools.at(i)->Finalise()){
	  //  if(m_verbose)*(m_data.Log)<<m_toolnames.at(i)<<" Finalised successfully"<<std::endl<<std::endl;
	  logmessage<<green<<m_toolnames.at(i)<<" Finalised successfully"<<plain<<std::endl;
	  m_data.Log->Log( logmessage.str(),2,m_verbose);
	  logmessage.str("");

	}
	else{
	  //  *(m_data.Log)<<"WARNING !!!!!!! "<<m_toolnames.at(i)<<" Finalised successfully (error code)"<<std::endl<<std::endl;;
	  logmessage<<red<<"WARNING !!!!!!! "<<m_toolnames.at(i)<<" Finalised successfully (error code)"<<plain<<std::endl;
	  m_data.Log->Log( logmessage.str(),0,m_verbose);
	  logmessage.str("");
	  
	  result=1;
	  if(m_errorlevel>1)exit(1);
	}  

#ifndef DEBUG      
      }
      
      catch(...){
	//*(m_data.Log)<<"WARNING !!!!!!! "<<m_toolnames.at(i)<<" Finalised successfully (uncaught error)"<<std::endl<<std::endl;
	logmessage<<red<<"WARNING !!!!!!! "<<m_toolnames.at(i)<<" Finalised successfully (uncaught error)"<<plain<<std::endl;
	m_data.Log->Log( logmessage.str(),0,m_verbose);
	logmessage.str("");
	
	result=2;
	if(m_errorlevel>0)exit(1);
      }
#endif

    }
    /*
      if(m_verbose){
      *(m_data.Log)<<"**** Tool chain Finalised ****"<<std::endl;
      *(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
      }
    */
    logmessage<<yellow<<"**** Toolchain Finalised ****"<<std::endl<<"********************************************************"<<plain<<std::endl;
    m_data.Log->Log( logmessage.str(),1,m_verbose);
    logmessage.str("");
    
    execounter=0;
    Initialised=false;
    Finalised=true;
    paused=false;
  }
  
  else {
    /*
    *(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
    *(m_data.Log)<<" ERROR: ToolChain Cannot Be Finalised As Has Not Been Initialised yet."<<std::endl;
    *(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
    */    

    logmessage<<purple<<"********************************************************"<<std::endl<<std::endl<<" ERROR: ToolChain Cannot Be Finalised As Has Not Been Initialised yet."<<std::endl<<"********************************************************"<<plain<<std::endl;
    m_data.Log->Log( logmessage.str(),0,m_verbose);
    logmessage.str("");

    result=-1;
  }



  return result;
}


void ToolChain::Interactive(){
  //  m_verbose=false;  
  exeloop=false;
  
  zmq::socket_t Ireceiver (*context, ZMQ_PAIR);
  Ireceiver.bind("inproc://control");
  
  ToolChainargs* args=new ToolChainargs;
  args->context=context;
  args->msgflag=&msgflag;

  pthread_create (&thread[0], NULL, ToolChain::InteractiveThread, args);

  bool running=true; 
  
  //zmq::pollitem_t in[]={{Ireceiver,0,ZMQ_POLLIN,0}};
  msgflag=false;
 
  while (running){
    
    zmq::message_t message;
    std::string command="";

    //    zmq::poll(in,1,0);
    //if(in[0].revents & ZMQ_POLLIN){
    if(msgflag){  
      //std::cout<<"in msgflag"<<std::endl;
      if(Ireceiver.recv (&message,ZMQ_NOBLOCK)){
	
	std::istringstream iss(static_cast<char*>(message.data()));
	iss >> command;
	
	// std::cout<<ExecuteCommand(command)<<std::endl<<std::endl;
	logmessage<<ExecuteCommand(command);
	printf("%s \n\n",logmessage.str().c_str());
	//printf("%s%s%s\n\n",".",command.c_str(),"."); 
	//m_data.Log->Log( logmessage.str(),0,m_verbose);
	logmessage.str("");
	if(command=="Quit"){
	  running=false;
	  //printf("%s \n","running false");
	}
	
	command="";
	//std::cout<<"Please type command : Start, Pause, Unpause, Stop, Quit (Initialise, Execute, Finalise)"<<std::endl;
	//std::cout<<">";
	
	logmessage<<"Please type command : "<<cyan<<"Start, Pause, Unpause, Stop, Status, Quit, ?, (Initialise, Execute, Finalise)"<<plain;
	//   m_data.Log->Log( logmessage.str(),0,m_verbose);
	printf("%s \n %s",logmessage.str().c_str(),">");
	logmessage.str("");
	//printf("%s \n","Received Quit");
	msgflag=false;
      }
    }
    //if(!running) printf("%s \n","before command execute");
    ExecuteCommand(command);
    //if(!running) printf("%s \n","after command execute");
  }
  pthread_join(thread[0], NULL);
  delete args;
  args=0;
  //printf("%s \n","out of running loop");
  
}  



std::string ToolChain::ExecuteCommand(std::string command){
  std::stringstream returnmsg;
  
  if(command!=""){
    if(command=="Initialise"){
      int ret=Initialise();
      if (ret==0)returnmsg<<"Initialising ToolChain";
      else returnmsg<<red<<"Error Code "<<ret<<plain;
    }
    else if (command=="Execute"){
      int ret=Execute();
      if (ret==0)returnmsg<<"Executing ToolChain";
      else returnmsg<<red<<"Error Code "<<ret<<plain;
    }
    else if (command=="Finalise"){
      exeloop=false;
      int ret=Finalise();
      if (ret==0)returnmsg<<"Finalising  ToolChain";
      else returnmsg<<red<<"Error Code "<<ret<<plain;
    }
    else if (command=="Quit"){
      returnmsg<<"Quitting";
      // if (interactive)exit(0);
    }
    else if (command=="Start"){
      int ret=Initialise();
      exeloop=true;
      execounter=0;
      if (ret==0)returnmsg<<"Starting ToolChain";
      else returnmsg<<red<<"Error Code "<<ret<<plain;
    }
    else if(command=="Pause"){
      exeloop=false;
      paused=true;
      returnmsg<<"Pausing ToolChain";
    }
    else if(command=="Unpause"){
      exeloop=true;
      paused=false;
      returnmsg<<"Unpausing ToolChain";
    }
    else if(command=="Stop") {
      exeloop=false;
      int ret=Finalise();
      if (ret==0)returnmsg<<"Stopping ToolChain";
      else returnmsg<<red<<"Error Code "<<ret<<plain;
    }
    
    else if(command=="Restart") {
      int ret=Finalise()+Initialise();
      if (ret==0)returnmsg<<"Restarting ToolChain";
      else returnmsg<<red<<"Error Code "<<ret<<plain;
    }
    
    else if(command=="Status"){
      std::stringstream tmp;
      if(Finalised) tmp<<"Waiting to Initialise ToolChain";
      if(Initialised && execounter==0) tmp<<"Initialised waiting to Execute ToolChain";
      if(Initialised && execounter>0){
	if(paused)tmp<<"ToolChain execution pasued";
	else tmp<<"ToolChain running (loop counter="<<execounter<<")";
      }
      
      if(*(m_data.vars["Status"])!="") tmp<<" : "<<*(m_data.vars["Status"]);
      returnmsg<<tmp.str();
    }
    else if(command=="?")returnmsg<<" Available commands: Initialise, Execute, Finalise, Start, Stop, Restart, Pause, Unpause, Quit, Status, ?";
    else if(command!=""){
      returnmsg<<purple<<"command not recognised please try again"<<plain;
    }
  }
  if(Finalised || (!Finalised && !exeloop)) usleep(100);
  if(exeloop) Execute();
  return returnmsg.str();
}




void ToolChain::Remote(int portnum, std::string SD_address, int SD_port){
  
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

void ToolChain::Remote(){
  
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



void* ToolChain::InteractiveThread(void* arg){

  ToolChainargs* args=static_cast<ToolChainargs*>(arg);
  //zmq::context_t * context = static_cast<zmq::context_t*>(arg);

  zmq::context_t * context=args->context;
  zmq::socket_t Isend (*context, ZMQ_PAIR);

  std::stringstream socketstr;
  Isend.connect("inproc://control");

  bool running=true;

  //  std::cout<<"Please type command : Start, Pause, Unpause, Stop, Quit (Initialise, Execute, Finalise)"<<std::endl;
  // std::cout<<">";

  printf("%s %s %s %s\n %s %s %s","Please type command :",cyan," Start, Pause, Unpause, Stop, Restart, Status, Quit, ?, (Initialise, Execute, Finalise)",plain,green,">",plain);
  /* logmessage<<"Please type command : Start, Pause, Unpause, Stop, Quit (Initialise, Execute, Finalise)"<<std::endl<<">";
  m_data.Log->Log( logmessage.str(),0,m_verbose);
  logmessage.str("");
  */

  while (running){

    std::string tmp;
    std::cin>>tmp;
    zmq::message_t message(tmp.length()+1);
    snprintf ((char *) message.data(), tmp.length()+1 , "%s" ,tmp.c_str()) ;
    Isend.send(message);
    *(args->msgflag)=true;
    if (tmp=="Quit")running=false;
  }
  //printf("quitting interactive threadd");

  pthread_exit(NULL);
  //return (NULL);

}


/*
bool ToolChain::Log(std::string message, int messagelevel, bool verbose){

  if(verbose){

    if (m_logging=="Interactive" && verboselevel <= m_verbose)std::cout<<"\
["<<verboselevel<<"] "<<message<<std::endl;

  }


}


static  void *LogThread(void* arg){



}

*/


ToolChain::~ToolChain(){
  
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

