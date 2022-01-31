#include "ServiceDiscovery.h"

ServiceDiscovery::ServiceDiscovery(bool Send, bool Receive, int remoteport, std::string address, int multicastport, zmq::context_t * incontext, boost::uuids::uuid UUID, std::string service, int pubsec, int kicksec){
 
    
  m_UUID=UUID;
  context=incontext;
  m_multicastport=multicastport;
  m_multicastaddress=address;
  m_service=service;
  m_remoteport=remoteport;
  m_send=Send;
  m_receive=Receive;

  args= new thread_args(m_UUID, context, m_multicastaddress, m_multicastport, m_service, m_remoteport, pubsec, kicksec);

    if (Receive) pthread_create (&thread[0], NULL, ServiceDiscovery::MulticastListenThread, args);
  
    if (Send) pthread_create (&thread[1], NULL, ServiceDiscovery::MulticastPublishThread, args);
  
  //sleep(2);
  
  
  //  zmq::socket_t ServiceDiscovery (*context, ZMQ_REQ);
  //ServiceDiscovery.connect("inproc://ServiceDiscovery");
  
 
  //zmq::socket_t ServicePublish (*context, ZMQ_PUSH);
  //ServicePublish.connect("inproc://ServicePublish");
  
}

ServiceDiscovery::ServiceDiscovery( std::string address, int multicastport, zmq::context_t * incontext, int kicksec){
  
  context=incontext;
  m_multicastport=multicastport;
  m_multicastaddress=address;
  m_service="none";
  m_remoteport=0;
  m_UUID=boost::uuids::random_generator()();
  m_receive=true;
  m_send=false;

  args= new thread_args(m_UUID, context, m_multicastaddress, m_multicastport, m_service, m_remoteport, 0 , kicksec);

  pthread_create (&thread[0], NULL, ServiceDiscovery::MulticastListenThread, args);

  //  sleep(2);
  

}


void* ServiceDiscovery::MulticastPublishThread(void* arg){
    

  thread_args* args= static_cast<thread_args*>(arg);
  zmq::context_t * context = args->context;
  boost::uuids::uuid m_UUID=args->UUID;
  std::string m_multicastaddress=args->multicastaddress;
  int m_multicastport=args->multicastport;
  std::string m_service=args->service;
  int m_remoteport=args->remoteport;

  long msg_id=0;

  zmq::socket_t Ireceive (*context, ZMQ_PULL);      
  int linger = 0;
  //Ireceive.setsockopt(ZMQ_IMMEDIATE, 1);
  Ireceive.setsockopt (ZMQ_LINGER, &linger, sizeof (linger)); 
  Ireceive.bind("inproc://ServicePublish");  
  /// multi cast /////
  
  
  struct sockaddr_in addr;
  int addrlen, sock, cnt;
  struct ip_mreq mreq;
  
  
  // set up socket //
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  struct linger l;
  l.l_onoff  = 0;
  l.l_linger = 0;
  setsockopt(sock, SOL_SOCKET, SO_LINGER,(char *) &l, sizeof(l));
	
  //fcntl(sock, F_SETFL, O_NONBLOCK); 
  if (sock < 0) {
    perror("socket");
    printf("Failed to connect to multicast publish socket");
    exit(1);
  }
  bzero((char *)&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(m_multicastport);
  addrlen = sizeof(addr);
  
  // send //
  addr.sin_addr.s_addr = inet_addr(m_multicastaddress.c_str());
  
  std::vector<Store> PubServices;
  
  Store bb; 
  *bb["msg_type"]="Service Discovery";
  bb.Set("msg_value",m_service);
  bb.Set("remote_port",m_remoteport);
  bb.Set("status_query",true);
  bb.Set("uuid",m_UUID);
  PubServices.push_back(bb);

  //  Initialize poll set
  zmq::pollitem_t items [] = {
    { Ireceive, 0, ZMQ_POLLIN, 0 },
    { NULL, sock, ZMQ_POLLOUT, 0 },
  };
  
  
  bool running=true;
  
  while(running){
  
    zmq::poll(&items [0], 2, -1);

    if ((items [0].revents & ZMQ_POLLIN) && running) {
      
      zmq::message_t commands;
      Ireceive.recv(&commands);
      
      std::istringstream tmp(static_cast<char*>(commands.data()));
      std::string command;
      std::string service;
      boost::uuids::uuid uuid;
      int port;
      bool statusquery;
      
      tmp>>command>>service>>uuid>>port>>statusquery;
     
     
      if(command=="Quit"){
	//printf("publish quitting \n");	
	running=false;
      }
      else if(command=="Add"){
	
	Store bb; 
	*bb["msg_type"]="Service Discovery";
	bb.Set("msg_value",service);
	bb.Set("remote_port",port);
	bb.Set("status_query",statusquery);
	bb.Set("uuid",uuid);
	PubServices.push_back(bb);

      }
      else if(command=="Delete"){
       	std::vector<Store>::iterator it;
	for (it = PubServices.begin() ; it != PubServices.end(); ++it){
	  //std::cout<<"d3.5 "<<*((*it)["msg_value"])<<std::endl;
	  
	  if(*((*it)["msg_value"])==service)break;
	  
	  
	}
	if (it!=PubServices.end())PubServices.erase(it);
	
	
      }
      
    }
    
    if ((items [1].revents & ZMQ_POLLOUT) && running){
      

      for(int i=0;i<PubServices.size();i++){

	//	char message[512];
	
	//      const char* json = "{\"uuid\":\"\",\"msg_id\":0,\"msg_time\":\"\",\"msg_type\":\"\",\"msg_value\":\"\",\"params\":{\"port\":0,\"status\":\"\"}}";
	
	
    
	/*
	  
	  
	  rapidjson::Document d;
	  d.Parse(json);
	  // d.SetObject();
	  //["hello"] = "rapidjson"; 
	  //   	test.SetString("My JSON Document", d.GetAllocator());
	  //  	d.AddMember("Doc Name", test , d.GetAllocator());
	  //	d.AddMember("uuid", tmp.str().c_str(), d.GetAllocator());
	  // (*newDoc)["Parameters"].SetObject();
	  
	  msg_id++;
	  std::stringstream tmp;
	  tmp<<m_UUID;
	  d["uuid"].SetString(tmp.str().c_str(), strlen(tmp.str().c_str()));
	  std::cout<<" UUID = "<<tmp.str().c_str()<<std::endl;
	  std::cout<<" 1d[UUID] = "<<d["uuid"].GetString()<<std::endl;
	  d["msg_id"].SetInt(msg_id);
	  std::cout<<" 2d[UUID] = "<<d["uuid"].GetString()<<std::endl;
	*/


	boost::posix_time::ptime t = boost::posix_time::microsec_clock::universal_time();
	std::stringstream isot;
	isot<<boost::posix_time::to_iso_extended_string(t) << "Z";

	/*
	  std::cout<<" 3d[UUID] = "<<d["uuid"].GetString()<<std::endl;
	  d["msg_time"].SetString(isot.str().c_str(), strlen(isot.str().c_str()));;
	  std::cout<<" 4d[UUID] = "<<d["uuid"].GetString()<<std::endl;
	  d["msg_type"]="Service Discovery";
	  d["msg_value"].SetString(m_service.c_str(),strlen(m_service.c_str()));
	  
	  std::cout<<" 5d[UUID] = "<<d["uuid"].GetString()<<std::endl;
	*/ 

	bool statusquery=false;
	PubServices.at(i).Get("status_query",statusquery);
	Store mm;


	if(statusquery){
	  
	  zmq::socket_t StatusCheck (*context, ZMQ_REQ);
	  //int a=12000;
	  //StatusCheck.setsockopt(ZMQ_RCVTIMEO, a);
	  //StatusCheck.setsockopt(ZMQ_SNDTIMEO, a);
	  std::stringstream connection;
	  connection<<"tcp://localhost:"<<*(PubServices.at(i)["remote_port"]);
	  // StatusCheck.setsockopt(ZMQ_IMMEDIATE, 1);
	   StatusCheck.setsockopt (ZMQ_LINGER, &linger, sizeof (linger)); 
	  
	  StatusCheck.connect(connection.str().c_str());
	
	  zmq::pollitem_t out[]={{StatusCheck,0,ZMQ_POLLOUT,0}};  
	  zmq::pollitem_t in[]={{StatusCheck,0,ZMQ_POLLIN,0}};

	  mm.Set("msg_type","Command");
	  mm.Set("msg_value","Status");
	  
	  std::string command;
	  mm>>command;
	  
	  // zmq::message_t Esend(256);
	  //std::string command="Status;
	
	  
	  zmq::message_t Esend(command.length()+1);
	  snprintf ((char *) Esend.data(), command.length()+1 , "%s" ,command.c_str()) ;
	 
	  zmq::poll(out,1,1000);
	 
	  if(out[0].revents & ZMQ_POLLOUT){
	    StatusCheck.send(Esend);
	    
	    
	    //std::cout<<"waiting for message "<<std::endl;
	    zmq::poll(in,1,1000);
	    if(in[0].revents & ZMQ_POLLIN){
	      zmq::message_t Ereceive;
	      StatusCheck.recv (&Ereceive);
	      std::istringstream ss(static_cast<char*>(Ereceive.data()));
	      
	      mm.JsonParser(ss.str());
	    }
	  }
	}
	/*
	std::cout<<"received for publish "<<ss.str()<<std::endl;
	std::cout<<" 6d[UUID] = "<<d["uuid"].GetString()<<std::endl;
	/*  rapidjson::Document params;
	
	// params=d["params"].GetType();
	std::cout<<d["params"].GetType()<<std::endl;
	std::cout<<d["params"].IsNull()<<std::endl;
	std::cout<<d["params"].IsFalse()<<std::endl;
	std::cout<<d["params"].IsTrue()<<std::endl;
	std::cout<<d["params"].IsBool()<<std::endl;
	std::cout<<d["params"].IsObject()<<std::endl;
	std::cout<<d["params"].IsArray()<<std::endl;
	std::cout<<d["params"].IsNumber()<<std::endl;
	std::cout<<d["params"].IsString()<<std::endl;
	
	// std::string tmpsubstring=d["params"].GetString();
	//std::cout<<"tmpsubstring "<<tmpsubstring<<std::endl;
	// params.Parse(tmpsubstring.c_str());
	
	std::cout<<"debug 1 "<<std::endl;
	
	params["port"].SetInt(m_multicastport);
	params["status"].SetString(ss.str().c_str(),strlen(ss.str().c_str()));
	
	rapidjson::StringBuffer subbuffer;
	rapidjson::Writer<rapidjson::StringBuffer> subwriter(subbuffer);
	params.Accept(subwriter);
	
	std::string tmpbufferout=subbuffer.GetString();
	
	std::cout<<" substringtest "<<tmpbufferout<<std::endl;
	
	d["params"].SetString(tmpbufferout.c_str(), strlen(tmpbufferout.c_str()));
	
	for (rapidjson::Value::ConstMemberIterator itr = d["params"].MemberBegin();itr != d["params"].MemberEnd(); ++itr){
	
	std::cout<<itr->name.GetString()<<std::endl;//<<" : "<< itr->value<<std::endl;
	//	if(itr->name.GetString()=="port") itr->value.SetInt(m_multicastport);
	//if(itr->name.GetString()=="status") itr->value.SetString(ss.str().c_str(),strlen(ss.str().c_str()));
	}
	  */
	  
	  
	  
	  //rapidjson::Document params=d["params"];
	  //params["port"].SetInt(m_multicastport,strlen();
	  // rapidjson::Value& params = d["params"]; 
	  // rapidjson::Document::AllocatorType& allocator = d.GetAllocator();
	  //    params.PushBack(i, allocator);
	  
	  // params.PushBack("key1","value1");
       //params.PushBack("key2","value2");
	  /* 
	     std::cout<<" d[UUID] = "<<d["uuid"].GetString()<<std::endl;
	     
	     rapidjson::StringBuffer buffer;
	     rapidjson::Writer<rapidjson::StringBuffer, rapidjson::Document::EncodingType, rapidjson::ASCII<> > writer(buffer);
	     //rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	     d.Accept(writer);
	     std::string hhh=buffer.GetString();
	     std::cout<< "bufer = "<<hhh<<std::endl;
	  */	
  
	  msg_id++;
	  
	  //	  PubServices.at(i).Set("uuid",m_UUID);
	  PubServices.at(i).Set("msg_id",msg_id);
	  *(PubServices.at(i))["msg_time"]=isot.str();
	  // *bb["msg_type"]="Service Discovery";
	  // bb.Set("msg_value",m_service);
	  //bb.Set("remote_port",m_remoteport);
	 	if(statusquery) *(PubServices.at(i))["status"]=*mm["msg_value"]; 
		else *(PubServices.at(i))["status"]= "N/A";
	  std::string pubmessage;
	  PubServices.at(i)>>pubmessage;
	  
	  //std::stringstream pubmessage;
	  
	  //pubmessage<<"{\"uuid\":\""<<m_UUID<<"\",\"msg_id\":"<<msg_id<<",\"msg_time\":\""<<isot.str()<<"\",\"msg_type\":\"Service Discovery\",\"msg_value\":\""<<m_service<<"\",\"params\":{\"port\":"<<m_remoteport<<",\"status\":\""<<ss.str()<<"\"}}";
	  char message[pubmessage.length()+1];

	  //    snprintf (message, 512 , "%s" , buffer.GetString()) ;
	  snprintf (message, pubmessage.length()+1 , "%s" , pubmessage.c_str() ) ;
	  //	  printf("sending: %s\n", message);
	   cnt = sendto(sock, message, sizeof(message), 0,(struct sockaddr *) &addr, addrlen);
	  
	  /*
	    if (cnt < 0) {
	    perror("sendto");
	    }
	  */
	  
 
      }
      
      sleep(args->pubsec);
      
    }
    
    
  }
  close(sock);
  Ireceive.close();
  //  printf("publish out of runnin \n");

  pthread_exit(NULL);
  //return (NULL);
  
  
}


void* ServiceDiscovery::MulticastListenThread(void* arg){

  thread_args* args= static_cast<thread_args*>(arg);
  zmq::context_t * context = args->context;
  boost::uuids::uuid m_UUID=args->UUID;
  std::string m_multicastaddress=args->multicastaddress;
  int m_multicastport=args->multicastport;
  std::string m_service=args->service;
  
  zmq::socket_t Ireceive (*context, ZMQ_ROUTER);
  Ireceive.bind("inproc://ServiceDiscovery");  
  

  /*
  zmq::message_t config;
  Ireceive.recv (&config);
  std::istringstream configuration(static_cast<char*>(config.data()));
  std::string group;
  int port;
  configuration>>group>>port;
  
  Ireceive.send(config);
  */

  ///// multi cast /////
  
 
  
  struct sockaddr_in addr;
  int addrlen, sock, cnt;
  struct ip_mreq mreq;
  char message[512];
  
  // set up socket //
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  int a =1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &a, sizeof(int));
  //fcntl(sock, F_SETFL, O_NONBLOCK); 
  if (sock < 0) {
    perror("socket");
    exit(1);
  }
  bzero((char *)&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(m_multicastport);
  addrlen = sizeof(addr);
  
  // receive //
  if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {        
    perror("bind");
    printf("Failed to bind to multicast listen socket");
    exit(1);
  }    
  mreq.imr_multiaddr.s_addr = inet_addr(m_multicastaddress.c_str());         
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);         
  if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,&mreq, sizeof(mreq)) < 0) {
    perror("setsockopt mreq");
    printf("Failed to goin multicast group listen thread");
    exit(1);
  }         
  
  
  //////////////////////////////
  
  zmq::pollitem_t items [] = {
    { NULL, sock, ZMQ_POLLIN, 0 },
    { Ireceive, 0, ZMQ_POLLIN, 0 }
  };

  zmq::pollitem_t out[] = {
    {Ireceive, 0, ZMQ_POLLOUT, 0}
  };
  
  std::map<std::string,Store*> RemoteServices;
  
  bool running=true;
  
  while(running){
    
    zmq::poll (&items [0], 2, -1);
    
    if ((items [0].revents & ZMQ_POLLIN) && running) {
      

      cnt = recvfrom(sock, message, sizeof(message), 0, (struct sockaddr *) &addr, (socklen_t*) &addrlen);
      if (cnt < 0) {
	//perror("recvfrom");
	// exit(1);
      } 
      else if (cnt > 0){
	//printf("%s: message = \"%s\"\n", inet_ntoa(addr.sin_addr), message);
	

	
	Store* newservice= new Store();
	newservice->Set("ip",inet_ntoa(addr.sin_addr));
	newservice->JsonParser(message);
	
	std::string uuid;
	newservice->Get("uuid",uuid);
	if(RemoteServices.count(uuid)) delete RemoteServices[uuid];
	
	RemoteServices[uuid]=newservice;
	
	
	//	std::cout<<" SD RemoteServices size = " << RemoteServices.size()<<std::endl;
	
      }
      

      
    }
    //	std::cout<<" SD RemoteServices size = " << RemoteServices.size()<<std::endl;

    std::vector<std::string> erase_list;
    
    for (std::map<std::string,Store*>::iterator it=RemoteServices.begin(); it!=RemoteServices.end(); ++it){ 
      //  if(*(it->second)[msg_time]==) delete;
      
      //std::string date_1 = "2014-08-15 10:12:10";
      //std::string now = "2014-08-15 16:40:02";
      
      std::string msg_time_orig;
      std::string msg_time_after;
      it->second->Get("msg_time",msg_time_orig);
      
      //std::cout<<" time orig ="<<msg_time_orig<<std::endl;
      
      for(std::string::size_type i = 0; i < msg_time_orig.size(); ++i) {
	if(msg_time_orig[i]!='-' && msg_time_orig[i]!=':') msg_time_after+=msg_time_orig[i];
      }
      //std::cout<<" time after ="<<msg_time_after<<std::endl;
      
      boost::posix_time::ptime t1 = boost::posix_time::microsec_clock::universal_time();
      
      //std::stringstream isot;
      //isot<<boost::posix_time::to_iso_extended_string(t) << "Z";
      
      boost::posix_time::ptime t2(boost::posix_time::from_iso_string(msg_time_after));
      
      //std::cout << "t1: " << t1 << std::endl;
      //std::cout << "t2: " << t2 << std::endl;
      
      boost::posix_time::time_duration td = t2 - t1;
	  
      tm td_tm = to_tm(td);
      //std::cout << boost::posix_time::to_iso_string(td) << std::endl;
      // std::cout<<"seconds = "<<td_tm.tm_sec<<std::endl; 
      // std::cout<<"mins = "<<td_tm.tm_min<<std::endl; 
       if(((td_tm.tm_min*60)+td_tm.tm_sec)>args->kicksec){
	 erase_list.push_back(it->first);
	 //	delete it->second;
	 //it->second=0;
	 //RemoteServices.erase(it->first);
	
      }
      //std::cout<< "uuid = "<<it->first<<std::endl;
    }
    
    for(int i=0;i<erase_list.size();i++){
      delete RemoteServices[erase_list.at(i)];
      RemoteServices[erase_list.at(i)]=0;
      RemoteServices.erase(erase_list.at(i));
    }
    erase_list.clear();

   
    if ((items [1].revents & ZMQ_POLLIN) && running) {
      
      zmq::message_t Identity;
      Ireceive.recv(&Identity);
      
      Ireceive.send(Identity,ZMQ_SNDMORE);

      zmq::message_t comm;
 
      if(Ireceive.recv(&comm)){

	std::istringstream iss(static_cast<char*>(comm.data()));
	std::string arg1="";
	std::string arg2="";

	iss>>arg1;

	if(arg1=="All"){
	
	  
	  //zmq::message_t sizem(512);
	   int size= RemoteServices.size();
	   zmq::message_t sizem(sizeof size);

	   snprintf ((char *) sizem.data(), sizeof size , "%d" ,size) ;
	  
	   //	   zmq::poll(out,1,1000);
	   
	   // if (out[0].revents & ZMQ_POLLOUT){ 
	     //std::cout<<"SD sent size="<<size<<std::endl;
	     //std::cout<<"SD sent message size="<<sizeof(sizem.data())<<std::endl;

	     if(size==0) Ireceive.send(sizem);
	     else Ireceive.send(sizem,ZMQ_SNDMORE);	    
	    
	     
	     for (std::map<std::string,Store*>::iterator it=RemoteServices.begin(); it!=RemoteServices.end(); ){
	    
	       std::string service;
	       *(it->second)>>service;
	       zmq::message_t send(service.length()+1);
	       snprintf ((char *) send.data(), service.length()+1 , "%s" ,service.c_str()) ;
	    
	       //	       zmq::poll(out,1,1000);
	    
	       //	       if (out[0].revents & ZMQ_POLLOUT){
	       it++;
	       if(it!=RemoteServices.end()){
		 Ireceive.send(send,ZMQ_SNDMORE);
		 //std::cout<<"SD sent a message"<<std::endl;

	       }
	       else {
		 Ireceive.send(send);
		 //std::cout<<"SD sent final message"<<std::endl;
	       }

		 // }
	       // else {
	       // break;
	       // }
	       
	     }
	     // }
	   
	}
	
	
	if(arg1=="Service"){
	  
	  iss>>arg1>>arg2;
	  
	  for (std::map<std::string,Store*>::iterator it=RemoteServices.begin(); it!=RemoteServices.end(); ++it){
	    
	    std::string test;
	    it->second->Get("service",test);
	    
	    if(arg2==test){
	      
	      std::string service;
	      *(it->second)>>service;
	      zmq::message_t send(service.length()+1);
	      snprintf ((char *) send.data(), service.length()+1 , "%s" ,service.c_str()) ;

	      zmq::poll(out,1,1000);
	      
	      if(out[0].revents & ZMQ_POLLOUT) Ireceive.send(send);	    
	    }
	  }
	  
	}


	
	if(arg1=="UUID"){

	  iss>>arg1>>arg2;
	  
	  for (std::map<std::string,Store*>::iterator it=RemoteServices.begin(); it!=RemoteServices.end(); ++it){
	    
	   
	    std::string test;
	    it->second->Get("uuid",test);
	    
	    if(arg2==test){
	      
	      std::string service;
	      *(it->second)>>service;
	      zmq::message_t send(service.length()+1);
	      snprintf ((char *) send.data(), service.length()+1 , "%s" ,service.c_str()) ;

	      zmq::poll(out,1,1000);

	      if(out[0].revents & ZMQ_POLLOUT) Ireceive.send(send);	    
	    }
	  }
	  
	}
	
	if(arg1=="Quit"){
	  
	  running=false;
	  //printf("quitting listening \n");	
	}
	
	
        
	
	/*
	std::string tmp="0";
	zmq::message_t send(tmp.length()+1);
	snprintf ((char *) send.data(), tmp.length()+1 , "%s" ,tmp.c_str()) ;
	Ireceive.send(send);
	//printf("sent \n");
	*/
      }
      
    }

	  
  }

  for (std::map<std::string,Store*>::iterator it=RemoteServices.begin(); it!=RemoteServices.end(); ++it){
    delete it->second;
    it->second=0;
    
  }
  RemoteServices.clear();
  //printf("exiting sd listen thread \n");
  pthread_exit(NULL);
  //return (NULL);
  
}

   

ServiceDiscovery::~ServiceDiscovery(){
  
   //printf("in sd destructor \n");
    sleep(1);  
  //printf("finnish sleep \n");
  
  // kill publish thread
  
  //printf("checking send \n");
  if (m_send){
    //printf("in sd send kill \n");
    zmq::socket_t ServicePublish (*context, ZMQ_PUSH);
    //int a=120000;
    //ServicePublish.setsockopt(ZMQ_RCVTIMEO, a);
    //ServicePublish.setsockopt(ZMQ_SNDTIMEO, a);
    ServicePublish.connect("inproc://ServicePublish");

    
    
    zmq::message_t command(5);
    snprintf ((char *) command.data(), 5 , "%s" ,"Quit") ;
    ServicePublish.send(command);
    
    //printf("send waiting fir join \n");
    pthread_join(thread[1], NULL);
    //printf("send joint \n");
  }
  
    //  zmq::socket_t ServiceDiscovery (*context, ZMQ_REQ);
    //ServiceDiscovery.connect("inproc://ServiceDiscovery");
  

    //zmq::socket_t ServicePublish (*context, ZMQ_PUSH);
    //ServicePublish.connect("inproc://ServicePublish");

    //kill listener //zmq::socket_t Ireceive (*context, ZMQ_DEALER);
    //Ireceive.bind("inproc://ServiceDiscovery");
  //printf("checking receive \n");
  if(m_receive){
  //printf("in sd receive kill \n");
    zmq::socket_t ServiceDiscovery (*context, ZMQ_DEALER);
    //  int a=60000;
    //ServiceDiscovery.setsockopt(ZMQ_RCVTIMEO, a);
    //ServiceDiscovery.setsockopt(ZMQ_SNDTIMEO, a);    
    ServiceDiscovery.connect("inproc://ServiceDiscovery");
  

    zmq::message_t command(5);
    snprintf ((char *) command.data(), 5 , "%s" ,"Quit") ;
    ServiceDiscovery.send(command);
  
    //printf("sent waiting for receive \n");
    //zmq::message_t ret;
    //ServiceDiscovery.recv(&ret);
      //std::istringstream tmp(static_cast<char*>(ret.data()));

    //printf("received waiting fir join \n");
    pthread_join(thread[0], NULL);
    //printf("received joint \n");

  }

  if(args!=0){
    //printf("in arg if \n");
    delete args;
    //printf("finnished arg delete \n");
    args=0;
    //printf("finnish Set args=0 \n");
  }
  //printf("deleted args \n");

  
  //printf("finnish sd destructor \n");
}


