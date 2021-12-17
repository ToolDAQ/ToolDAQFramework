#include <Utilities.h>

Utilities::Utilities(zmq::context_t* zmqcontext){ 
  context=zmqcontext;
  Threads.clear();
}

bool Utilities::AddService(std::string ServiceName, unsigned int port, bool StatusQuery){

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


bool Utilities::RemoveService(std::string ServiceName){

  zmq::socket_t Ireceive (*context, ZMQ_PUSH);
  Ireceive.connect("inproc://ServicePublish");

  std::stringstream test;
  test<<"Delete "<< ServiceName << " ";
  zmq::message_t send(test.str().length()+1);
  snprintf ((char *) send.data(), test.str().length()+1 , "%s" ,test.str().c_str()) ;

  return Ireceive.send(send);

}

int Utilities::UpdateConnections(std::string ServiceName, zmq::socket_t* sock, std::map<std::string,Store*> &connections, std::string port){

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
      service->Get("msg_value",type);
      service->Get("uuid",uuid);
      service->Get("ip",ip);
      if(port=="") service->Get("remote_port",port);
      std::string tmp=ip + ":" + port;

      //if(type == ServiceName && connections.count(uuid)==0){
      if(type == ServiceName && connections.count(tmp)==0){
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

Thread_args* Utilities::CreateThread(std::string ThreadName,  void (*func)(Thread_args*), Thread_args* args){
  
  if(Threads.count(ThreadName)==0){
    
    if(args==0) args = new Thread_args();  
    
    args->context=context;
    args->ThreadName=ThreadName;
    args->func=func;
    args->running=true;
    
    pthread_create(&(args->thread), NULL, Utilities::Thread, args);
    
    args->sock=0;
    Threads[ThreadName]=args;
    
}

  else args=0;

  return args;
  
}


Thread_args* Utilities::CreateThread(std::string ThreadName,  void (*func)(std::string)){
  Thread_args *args =0;
  
  if(Threads.count(ThreadName)==0){
    
    args = new Thread_args(context, ThreadName, func);
    pthread_create(&(args->thread), NULL, Utilities::String_Thread, args);
    args->sock=0;
    args->running=true;
    Threads[ThreadName]=args;
  }
  
  return args;
}                    

void *Utilities::String_Thread(void *arg){
  

    Thread_args *args = static_cast<Thread_args *>(arg);

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

void *Utilities::Thread(void *arg){
  
  Thread_args *args = static_cast<Thread_args *>(arg);

  while (!args->kill){
    
    if(args->running) args->func(args );
    else usleep(100);
  
  }
  
  pthread_exit(NULL);

}

bool Utilities::MessageThread(Thread_args* args, std::string Message, bool block){ 

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

bool Utilities::MessageThread(std::string ThreadName, std::string Message, bool block){

  return MessageThread(Threads[ThreadName],Message,block);
}

bool Utilities::KillThread(Thread_args* &args){
  
  bool ret=false;
  
  if(args){
    
    args->running=false;
    args->kill=true;
    
    pthread_join(args->thread, NULL);
    //delete args;
    //args=0;
    
    
  }
  
  return ret;   
  
}

bool Utilities::KillThread(std::string ThreadName){
  
  return KillThread(Threads[ThreadName]);

}

