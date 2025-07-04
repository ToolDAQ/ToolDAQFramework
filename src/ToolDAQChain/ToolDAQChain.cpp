#include "ToolDAQChain.h"

using namespace ToolFramework;

ToolDAQChain::ToolDAQChain(std::string configfile, DataModel* data_model, int argc, char* argv[]): ToolChain(){

  m_data=reinterpret_cast<DataModelBase*>(data_model);
  m_DAQdata=reinterpret_cast<DAQDataModelBase*>(data_model);

  if(!m_data->vars.Initialise(configfile)){
    std::cout<<"\033[38;5;196m ERROR!!!: No valid config file quitting \033[0m"<<std::endl;
    exit(1);
  }

  if(!m_data->vars.Get("UUID_path",m_UUID_path)) m_UUID_path="/dev/random";
  if(!m_data->vars.Get("verbose",m_verbose)) m_verbose=9;
  if(!m_data->vars.Get("error_level",m_errorlevel)) m_errorlevel=2;
  if(!m_data->vars.Get("remote_port",m_remoteport)) m_remoteport=24004; 
  if(!m_data->vars.Get("attempt_recover",m_recover)) m_recover=false;

  if(!m_data->vars.Get("log_interactive",m_log_interactive)) m_log_interactive=true;
  if(!m_data->vars.Get("log_local",m_log_local)) m_log_local=false;
  if(!m_data->vars.Get("log_split_files",m_log_split_files)) m_log_split_files=false;
  if(!m_data->vars.Get("log_local_path",m_log_local_path)) m_log_local_path="./log";
  bool log_append_time=false;
  m_data->vars.Get("log_append_time", log_append_time);
  if(log_append_time){
    std::stringstream tmp;
    tmp<<m_log_local_path<<"."<<time(NULL);
    m_log_local_path= tmp.str();
  }
  if(!m_data->vars.Get("log_remote",m_log_remote)) m_log_remote=false; 
  if(!m_data->vars.Get("log_address",m_log_address)) m_log_address="239.192.1.1";
  if(!m_data->vars.Get("log_port",m_log_port)) m_log_port=5001;

  if(!m_data->vars.Get("service_discovery_address",m_multicastaddress)){
    std::string tmp_address="";
    if(m_data->vars.Get("service_discovery_address",tmp_address)) m_multicastaddress.emplace_back(tmp_address);
    else  m_multicastaddress.emplace_back("239.192.1.1");
  }
  if(!m_data->vars.Get("service_discovery_port",m_multicastport)){
    int tmp_port=0;
    if(m_data->vars.Get("service_discovery_port",tmp_port)) m_multicastport.emplace_back(tmp_port);
    else m_multicastport.emplace_back(5000);
  }
  if(!m_data->vars.Get("service_name",m_service)) m_service="ToolDAQ_Service";
  if(!m_data->vars.Get("service_publish_sec",m_pub_sec)) m_pub_sec=5;
  if(!m_data->vars.Get("service_kick_sec",m_kick_sec)) m_kick_sec=60;
  if(!m_data->vars.Get("use_backend_services",m_backend_services)) m_backend_services=false;
  
  if(!m_data->vars.Get("Inline", m_inline))  m_inline=0;
  if(!m_data->vars.Get("Interactive", m_interactive)) m_interactive=false;
  if(!m_data->vars.Get("Remote", m_remote)) m_remote=false;
  
  
  unsigned int IO_Threads=1;
  m_data->vars.Get("IO_Threads",IO_Threads);
  m_data->vars.Set("argc",argc);
  
  for(int i=0; i<argc ; i++){
    std::stringstream tmp;
    tmp<<"$"<<i;
    m_data->vars.Set(tmp.str(),argv[i]);
    
  }
 
  Init(IO_Threads);
  
  std::string toolsfile="";
  m_data->vars.Get("Tools_File",toolsfile);
  
  if(!LoadTools(toolsfile)) exit(1);
 
  if(m_inline!=0) Inline();  
  else if(m_interactive) Interactive(); 
  else if(m_remote) Remote();
  
}

ToolDAQChain::ToolDAQChain(int verbose, int errorlevel, std::string service, bool interactive, bool local, std::string log_local_path, bool remote, std::string log_address, int log_port,  bool split_output_files, int pub_sec, int kick_sec,unsigned int IO_Threads, DataModel* in_data_model){
  
  m_verbose=verbose;
  m_errorlevel=errorlevel;
  m_service=service;  
  m_log_interactive=interactive;
  m_log_local=local;
  m_log_local_path=log_local_path;
  m_log_remote=remote;
  m_log_address=log_address;
  m_log_port=log_port;
  m_log_split_files=split_output_files;  
  m_pub_sec=pub_sec;
  m_kick_sec=kick_sec;
  if(in_data_model==0) m_data=new DAQDataModelBase;
  else{
    m_data=reinterpret_cast<DataModelBase*>(in_data_model);
    m_DAQdata=reinterpret_cast<DAQDataModelBase*>(in_data_model);
  }
  
  
  Init(IO_Threads);  
  
}

void ToolDAQChain::Init(unsigned int IO_Threads){
 
  context=new zmq::context_t(IO_Threads);
  m_DAQdata->context=context;

  struct stat fbuffer;
  if(stat (m_UUID_path.c_str(), &fbuffer) == 0){ //if file exists

    BinaryStream tmp;
    tmp.Bopen(m_UUID_path.c_str(),READ, UNCOMPRESSED);
    tmp.Bread(&m_UUID, sizeof(m_UUID));
    tmp.Bclose();
  }

  else{

    m_UUID = boost::uuids::random_generator()();

    BinaryStream tmp;
    tmp.Bopen(m_UUID_path.c_str(), NEW, UNCOMPRESSED);
    tmp.Bwrite(&m_UUID, sizeof(m_UUID));
    tmp.Bclose();
    
  }

  m_data->vars.Set("UUID", m_UUID);
  
  m_log=0;

  m_log= new DAQLogging(context, m_UUID, m_service, m_log_interactive, m_log_local, m_log_local_path, m_log_remote, m_log_address, m_log_port, m_log_split_files);


  if(!m_data->Log) m_data->Log=m_log;

  /*  
  if(m_log_mode!="Off"){
    std::cout<<"in off if"<<std::endl;    
    bcout=std::cout.rdbuf();
    out=new  std::ostream(bcout);
  }
  */
  //  m_data->Log= new DAQLogging(*out, context, m_UUID, m_service, m_log_mode, m_log_local_path, m_log_service, m_log_port);
  
  //if(m_log_mode!="Off") std::cout.rdbuf((m_data->Log->buffer));
  
  execounter=0;
  Initialised=false;
  Finalised=true;
  paused=false;
  
  msg_id=0;
  
  bool sendflag=true;
  bool receiveflag=true;
  
//  if(m_pub_sec<0 || m_inline>0 || m_interactive) sendflag=false;
  if(m_inline>0 || m_interactive) m_remoteport=0;
  if(m_pub_sec<0) sendflag=false;
  if(m_kick_sec<0) receiveflag=false;

  SD=new ServiceDiscovery(sendflag,receiveflag, m_remoteport, m_multicastaddress,m_multicastport,context,m_UUID,m_service,m_pub_sec,m_kick_sec);
 
  if(m_remote) sleep(10); //needed to allow service discovery find time
  
  //  *m_log<<MsgL(1,m_verbose)<<cyan<<"UUID = "<<m_UUID<<std::endl<<yellow<<"\n********************************************************\n"<<"**** Tool chain created ****\n"<<"********************************************************"<<std::endl;
  *m_log<<MsgL(1,m_verbose)<<cyan<<"UUID = "<<m_UUID<<yellow<<"\n********************************************************\n"<<"**** Tool chain created ****\n"<<"********************************************************"<<std::endl;

  if(m_backend_services){
    m_DAQdata->services= new Services();
    m_DAQdata->services->Init(m_data->vars, m_DAQdata->context, &m_DAQdata->sc_vars, true);
  }
  
}





void ToolDAQChain::Remote(int portnum, std::string SD_address, int SD_port){

  std::vector<int> in_multicastport;
  std::vector<std::string> in_multicastaddress;
  in_multicastport.emplace_back(SD_port);
  in_multicastaddress.emplace_back(SD_address);
  Remote(portnum, in_multicastaddress, in_multicastport);

}

void ToolDAQChain::Remote(int portnum, std::vector<std::string> SD_address, std::vector<int> SD_port){
  
  m_remoteport=portnum;
  m_multicastport=SD_port;
  m_multicastaddress=SD_address;
  
  bool sendflag=true;
  bool receiveflag=true;
  
  if(m_pub_sec<0) sendflag=false;
  if(m_kick_sec<0) receiveflag=false;
  
  SD=new ServiceDiscovery(sendflag,receiveflag, m_remoteport, m_multicastaddress,m_multicastport,context,m_UUID,m_service,m_pub_sec,m_kick_sec);
  
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
      if(rr.Get<std::string>("msg_type")=="Command") command=rr.Get<std::string>("msg_value");
      else command="NULL";


       boost::posix_time::ptime t = boost::posix_time::microsec_clock::universal_time();
      std::stringstream isot;
      isot<<boost::posix_time::to_iso_extended_string(t) << "Z";

      msg_id++;
      Store bb;

      bb.Set("uuid",boost::uuids::to_string(m_UUID));
      bb.Set("msg_id",msg_id);
      bb.Set("msg_time", isot.str());
      bb.Set("msg_type", "Command Reply");
      bb.Set("msg_value",ExecuteCommand(command));

      std::string var1="";
      rr.Get("var1",var1);
      
      if(command =="?" && var1=="JSON"){
	
	std::string in= bb.Get<std::string>("msg_value");
	std::string out="[";
	std::vector<std::string> commands;
	std::string tmp="";
	for( unsigned int i=20; i<in.length(); i++){
	  if(in[i]!=',' && in[i]!=' ') tmp+=in[i];
	  if(in[i]==',' || (i==in.length()-1)){
	    commands.push_back(tmp);
	    tmp="";
	  }
	}
	for(unsigned int i=0; i<commands.size(); i++){
	  if(i!=0)out+=',';
	  out+="{\"name\":\"" + commands.at(i) + "\",\"type\":\"BUTTON\"}";	    	  
	}
	
	out+="]";
	bb.Set("msg_value",out);
	bb.Destring("msg_value");
      }
      
      
      
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
  
  /*  for (int i=0;i<m_tools.size();i++){
      delete m_tools.at(i);
      m_tools.at(i)=0;
      }
      
      printf("yakka2\n");  
      m_tools.clear();
      printf("yakka3\n");
  */ 

  //  m_DAQdata->sc_vars.Stop();
  delete m_DAQdata->services;
  m_DAQdata->services=0;
  
  delete m_log;
  m_log=0;
  
  //  printf("%s \n","tdebug 3");  
    
  delete SD;
  //printf("%s \n","tdebug 4");
  SD=0;  
  //printf("%s \n","tdebug 5");

  //printf("%s \n","tdebug 6");  
  delete context;
  //printf("%s \n","tdebug 7");
  context=0;
  
  //printf("%s \n","tdebug 8");
  
  /*
  if(m_data!=0 && m_data->Log==m_log){    
    m_data->Log=0;  
    delete m_data;      
    m_data=0;     
  }
  */
  delete m_DAQdata;
  m_DAQdata=0;
  m_data=0;

  /*
  delete m_log;
  m_log=0;
  */
  
  //  printf("%s \n","tdebug 1");
  //  delete m_data->Log;   // change this to m_log
  //printf("%s \n","tdebug 2");
  //m_data->Log=0;
  //sleep(30);
  //printf("%s \n","tdebug 6");
  //  context->close();
  //printf("%s \n","tdebug 6.5");

  // if(m_log_mode!="Off"){
  //std::cout.rdbuf(bcout);
  //printf("%s \n","tdebug 9");
  //delete out;
  //printf("%s \n","tdebug 10");
  //out=0;
  //printf("%s \n","tdebug 11");
  //  }  
  
  
  
}

