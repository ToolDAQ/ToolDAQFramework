#include <DAQUtilities.h>

DAQUtilities::DAQUtilities(zmq::context_t* zmqcontext){ 
  context=zmqcontext;
  Threads.clear();
}

bool DAQUtilities::AddService(std::string ServiceName, unsigned int port, bool StatusQuery){

  zmq::socket_t Ireceive (*context, ZMQ_PUSH);
  Ireceive.connect("inproc://ServicePublish");

  boost::uuids::uuid m_UUID;
  m_UUID = boost::uuids::random_generator()();

  std::stringstream test;
  test<<"Add "<< ServiceName <<" "<<m_UUID<<" "<<port<<" "<<((int)StatusQuery) ;

  zmq::message_t send(test.str().length()+1);
  snprintf ((char *) send.data(), test.str().length()+1 , "%s" ,test.str().c_str()) ;
 
  return Ireceive.send(send);


}


bool DAQUtilities::RemoveService(std::string ServiceName){

  zmq::socket_t Ireceive (*context, ZMQ_PUSH);
  Ireceive.connect("inproc://ServicePublish");

  std::stringstream test;
  test<<"Delete "<< ServiceName << " ";
  zmq::message_t send(test.str().length()+1);
  snprintf ((char *) send.data(), test.str().length()+1 , "%s" ,test.str().c_str()) ;

  return Ireceive.send(send);

}

int DAQUtilities::UpdateConnections(std::string ServiceName, zmq::socket_t* sock, std::map<std::string,Store*> &connections, std::string port){

    boost::uuids::uuid m_UUID=boost::uuids::random_generator()();
    long msg_id=0;

    zmq::socket_t Ireceive (*context, ZMQ_DEALER);
    Ireceive.connect("inproc://ServiceDiscovery");


    zmq::message_t send(4);
    snprintf ((char *) send.data(), 4 , "%s" ,"All") ;


    Ireceive.send(send);

    zmq::message_t receive;
    Ireceive.recv(&receive);
    std::istringstream iss(static_cast<char*>(receive.data()));

    int size;
    iss>>size;

    for(int i=0;i<size;i++){

      Store *service = new Store;

      zmq::message_t servicem;
      Ireceive.recv(&servicem);

      std::istringstream ss(static_cast<char*>(servicem.data()));
      service->JsonParser(ss.str());

      std::string type;
      std::string uuid;
      std::string ip;
      std::string remote_port;
      service->Get("msg_value",type);
      service->Get("uuid",uuid);
      service->Get("ip",ip);
      if(port=="") service->Get("remote_port",remote_port);
      else remote_port=port;      
      std::string tmp=ip + ":" + remote_port;


      if((type == ServiceName || ServiceName=="") && connections.count(tmp)==0){
	connections[tmp]=service;
	//std::string ip;
	//std::string port;
	//service->Get("ip",ip);
	//service->Get("remote_port",port);
	tmp="tcp://"+ tmp;
	sock->connect(tmp.c_str());
      }
      else{
	delete service;
	service=0;
      }


    }

    return connections.size();
  }



DAQThread_args* DAQUtilities::CreateThread(std::string ThreadName,  void (*func)(std::string)){
  DAQThread_args *args =0;
  
  if(Threads.count(ThreadName)==0){
    
    args = new DAQThread_args(context, ThreadName, func);
    pthread_create(&(args->thread), NULL, DAQUtilities::String_Thread, args);
    args->sock=0;
    args->running=true;
    Threads[ThreadName]=args;
  }
  
  return args;
}                    

void *DAQUtilities::String_Thread(void *arg){
  

    DAQThread_args *args = static_cast<DAQThread_args *>(arg);

    zmq::socket_t IThread(*(args->context), ZMQ_PAIR);
    /// need to subscribe
    std::stringstream tmp;
    tmp<<"inproc://"<<args->ThreadName;
    IThread.bind(tmp.str().c_str());


    zmq::pollitem_t initems[] = {
      {IThread, 0, ZMQ_POLLIN, 0}};

    args->running = true;

    while(!args->kill){
      if(args->running){
	
	std::string command="";
	
	zmq::poll(&initems[0], 1, 0);
	
	if ((initems[0].revents & ZMQ_POLLIN)){
	  
	  zmq::message_t message;
	  IThread.recv(&message);
	  command=std::string(static_cast<char *>(message.data()));
  	  
	}
	
	args->func_with_string(command);
      }

      else usleep(100);

    }
    
    pthread_exit(NULL);
  }


bool DAQUtilities::MessageThread(DAQThread_args* args, std::string Message, bool block){ 

  bool ret=false; 

  if(args){
    
    if(!args->sock){
      
      args->sock = new zmq::socket_t(*(args->context), ZMQ_PAIR);
      std::stringstream tmp;
      tmp<<"inproc://"<<args->ThreadName;
      args->sock->connect(tmp.str().c_str());
      
    }
    
    zmq::message_t msg(Message.length()+1);
    snprintf((char *)msg.data(), Message.length()+1, "%s", Message.c_str());
   
    if(block) ret=args->sock->send(msg);
    else ret=args->sock->send(msg, ZMQ_NOBLOCK);
    
  }
  
  return ret;
  
}

bool DAQUtilities::MessageThread(std::string ThreadName, std::string Message, bool block){

  return MessageThread((DAQThread_args*) Threads[ThreadName],Message,block);
}


Thread_args* DAQUtilities::ZMQProxy(std::string name, zmq::socket_t* from_sock, zmq::socket_t* to_sock){

  if(!from_sock || !to_sock) return 0;

  Proxy_Thread_args* args= new Proxy_Thread_args();

  args->from_sock = from_sock;
  args->to_sock = to_sock;
  //args->control = new zmq::socket_t(*(context), ZMQ_SUB); //////////for some reason proxy gives bad address
  //args->control->setsockopt(ZMQ_SUBSCRIBE, "", 0);
  //args->started=false;

  args->items[0].socket=*(args->from_sock);
  args->items[0].fd=0;
  args->items[0].events=ZMQ_POLLIN;
  args->items[0].revents=0;

  return CreateThread(name, &Proxy_Thread, args);
 
}

bool DAQUtilities::KillZMQProxy(Thread_args* arg){
  
  if(!arg) return false;
  
  Proxy_Thread_args* args =  static_cast<Proxy_Thread_args*>(arg);

  //  zmq::message_t message(9); //////////for some reason proxy gives bad address
  //snprintf((char*) message.data(), 9 , "%s" ,"TERMINATE") ;

  // args->control->send(message);

  if(!KillThread(args)) return false;
  
  
  args->from_sock=0;
  args->to_sock=0;
  delete args->control;
  args->control=0;

  delete args;
  args=0;
  
  return true;
  
}

void DAQUtilities::Proxy_Thread(Thread_args *arg){

 Proxy_Thread_args* args =  reinterpret_cast<Proxy_Thread_args*>(arg);

 // if(!args->started){      //////////for some reason proxy gives bad address
 // args->started=true;
   //   zmq::proxy(*(args->from_sock), *(args->to_sock), NULL);
   //zmq::proxy_steerable(args->from_sock, args->to_sock, NULL , args->control);
 //}

 zmq::poll(&(args->items[0]), 1, 100);

 if (args->items[0].revents & ZMQ_POLLIN){

   zmq::message_t msg;
   args->from_sock->recv(&msg);
   if(msg.more()) args->to_sock->send(msg, ZMQ_SNDMORE);
   else args->to_sock->send(msg);
 }


}
