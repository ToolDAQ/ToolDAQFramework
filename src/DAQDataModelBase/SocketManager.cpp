#include <SocketManager.h>

using namespace ToolFramework;

SocketManager_args::SocketManager_args():Thread_args(){}

SocketManager_args::~SocketManager_args(){

}

SocketManager::SocketManager(){}

SocketManager::~SocketManager(){

  Close();
  

}



bool SocketManager::Init(DataModel* data_model, zmq::socket_t* in_sock, Buffer<std::shared_ptr<ZMQMessages> >* receive_buffer, Buffer<ZMQMessages* >* send_buffer, Buffer<std::shared_ptr<ZMQMessages> >* bad_buffer, Pool<ZMQMessages>* message_pool, uint16_t expected_number_messages){

  args.shared_received_messages = receive_buffer;
  args.shared_bad_messages = bad_buffer;

  return Init(data_model, in_sock, (Buffer<ZMQMessages* >*)0, send_buffer, 0, message_pool,expected_number_messages);
  
  return true;
}

bool SocketManager::Init(DataModel* data_model, zmq::socket_t* in_sock, Buffer<std::shared_ptr<ZMQMessages> >* receive_buffer, Buffer<ZMQMessages* >* send_buffer, Buffer<ZMQMessages* >* bad_buffer, Pool<ZMQMessages>* message_pool, uint16_t expected_number_messages){

  args.shared_received_messages = receive_buffer;

  return Init(data_model, in_sock, (Buffer<ZMQMessages* >*)0, send_buffer, bad_buffer, message_pool,expected_number_messages);
  
  return true;
}

bool SocketManager::Init(DataModel* data_model, zmq::socket_t* in_sock, Buffer<ZMQMessages* >* receive_buffer, Buffer<ZMQMessages* >* send_buffer, Buffer<ZMQMessages* >* bad_buffer, Pool<ZMQMessages>* message_pool, uint16_t expected_number_messages){

  if(in_sock == 0 || data_model == 0 || message_pool ==0) return false; 
  m_util = new DAQUtilities(data_model->context);
  sock = in_sock;
  m_data = data_model;
  args.m_data = m_data;
  args.sock = in_sock;
  args.received_messages = receive_buffer;
  args.messages_to_send = send_buffer;
  args.bad_messages = bad_buffer;
  args.expected_number_messages = expected_number_messages;
  args.message_pool = message_pool;

  paused=false;
  CreateThread();

  return true;
}

void SocketManager::CreateThread(){


  args.sock_mtx = &sock_mtx;

  args.items[0].socket=*args.sock;
  args.items[0].fd=0;
  args.items[0].events=ZMQ_POLLIN;
  args.items[0].revents=0;
  args.items[1].socket=*args.sock;
  args.items[1].fd=0;
  args.items[1].events=ZMQ_POLLOUT;
  args.items[1].revents=0;
  
  num_messages_received = 0;
  num_replies_sent = 0;
  num_receive_errors = 0;
  num_send_errors = 0;
  args.num_messages_received = &num_messages_received;
  args.num_replies_sent = &num_replies_sent;
  args.num_receive_errors = &num_receive_errors;
  args.num_send_errors = &num_send_errors;

  args.paused = &paused;

  m_util->CreateThread("SocketManager", &Thread, &args);

}

void SocketManager::Thread(Thread_args* arg){

  SocketManager_args* args=reinterpret_cast<SocketManager_args*>(arg);

  while(*args->paused) usleep(1);

  if(args->messages_to_send !=0)  args->messages_to_send->Swap(args->local_messages_to_send);

  std::unique_lock<std::mutex> sock_lock(*args->sock_mtx);
  //////////////////////////////// waiting for incoming socket data ////////////
  try{
    if(args->local_messages_to_send.size() >0){
      if(args->received_messages !=0) args->poll_return = zmq::poll(&(args->items[0]), 2, 100);
      else args->poll_return = zmq::poll(&(args->items[1]), 1, 100);
    }
    else if(args->received_messages !=0) args->poll_return = zmq::poll(&(args->items[0]), 1, 100);
    else{
      sock_lock.unlock();
      usleep(100);
      return;
    }
    
  }
  catch(...){
    *(args->m_data->Log)<<"ERROR: Socket Manager poll failed"<<std::endl;
    if(sock_lock.owns_lock()) sock_lock.unlock();
    return;
  }
  /////////////////////////////////// receive data //////////////////
  
  while(args->poll_return >0 && args->items[0].revents & ZMQ_POLLIN){ 
    //  if(args->items[0].revents & ZMQ_POLLIN){ //data coming in on socket
    try{
     
      //receiving data
      args->local_received_messages = args->message_pool->GetNew();
      args->local_received_messages->messages.emplace_back();
      args->return_check = args->sock->recv(&args->local_received_messages->messages.back());
      while(args->return_check && args->local_received_messages->messages.back().more()){
	args->local_received_messages->messages.emplace_back();
	args->return_check = args->return_check && args->sock->recv(&args->local_received_messages->messages.back());
      }
      sock_lock.unlock();
      // check if number of messages is correct.
      if(!args->return_check || (args->local_received_messages->messages.size()!=args->expected_number_messages && args->expected_number_messages > 0)){
	//printf("d1\n");
	if(args->bad_messages == 0){
	  args->local_received_messages->messages.clear();
	  args->message_pool->Add(args->local_received_messages); // adding message vector back to pool rather than deleting to save instansiations
	}
	else{
	  if(args->shared_bad_messages != 0 ){
	    Pool<ZMQMessages >* pool = args->message_pool;
	    args->shared_bad_messages->Add(std::shared_ptr<ZMQMessages>(args->local_received_messages,[pool](ZMQMessages* p){pool->Add(p);}));
	  }
	  else args->bad_messages->Add(args->local_received_messages);


	}
	args->local_received_messages = 0;
	*(args->m_data->Log)<<"INFO: Socket Manager received bad number of message parts"<<std::endl;
	//	args->m_data->services->SendLog("Info: received bad data", LogLevel::Message);
	(*args->num_receive_errors)++;
      }
      else{
	if( args->shared_received_messages !=0){
	  Pool<ZMQMessages >* pool = args->message_pool;
	  args->shared_received_messages->Add(std::shared_ptr<ZMQMessages>(args->local_received_messages,[pool](ZMQMessages* p){pool->Add(p);}));
	}
	else args->received_messages->Add(args->local_received_messages);
	args->local_received_messages=0;
	(*args->num_messages_received)++;
      }
      sock_lock.lock();   
      args->poll_return = zmq::poll(&(args->items[0]), 1, 0);   
    }
    catch(...){
      *(args->m_data->Log)<<"INFO: Socket Manager caught error in receive"<<std::endl;
      if(sock_lock.owns_lock()) sock_lock.unlock();
    }
  }
  if(sock_lock.owns_lock()) sock_lock.unlock();
  
  //else  sock_lock.unlock();
  ///////////////////////////////////////////////////////////////////////////////////////////
  
  
  
  //////////////////////////////////////// Sending replies ///////////////////////
  if(args->items[1].revents & ZMQ_POLLOUT && args->local_messages_to_send.size() >0){ 
    size_t i=0;
    
    try{
      for(i=0; i < args->local_messages_to_send.size(); i++){
	sock_lock.lock();
	
	args->return_check = true;
	for(size_t j=0; j < args->local_messages_to_send.at(i)->messages.size()-1; j++){
	  
	  if(!args->sock->send(args->local_messages_to_send.at(i)->messages.at(j), ZMQ_SNDMORE)){
	    args->return_check = false;
	    break;
	  }
	  
	}
	
	args->return_check =  args->return_check && args->sock->send(args->local_messages_to_send.at(i)->messages.at(args->local_messages_to_send.at(i)->messages.size()-1));    
	
	if(!args->return_check){
	  *(args->m_data->Log)<<"INFO: Socket Manager error sending response"<<std::endl;
	  //	  args->m_data->services->SendLog("Info: error sending response", LogLevel::Message);
	  (*args->num_send_errors)++;
	  if(args->local_messages_to_send.at(i)->sent) *(args->local_messages_to_send.at(i)->sent)=false;
	  if(args->local_messages_to_send.at(i)->error) *(args->local_messages_to_send.at(i)->error)=true;
	  
	}
	else{
	  (*args->num_replies_sent)++;
	  if(args->local_messages_to_send.at(i)->sent) *(args->local_messages_to_send.at(i)->sent)=true;
	  if(args->local_messages_to_send.at(i)->error) *(args->local_messages_to_send.at(i)->error)=false;
	}
	sock_lock.unlock();
      }
    }
    catch(...){
      *(args->m_data->Log)<<"INFO: Socket Manager caught error in send"<<std::endl;
      if(sock_lock.owns_lock()) sock_lock.unlock();
      (*args->num_send_errors)++;
      if(args->local_messages_to_send.at(i)->sent) *(args->local_messages_to_send.at(i)->sent)=false;
      if(args->local_messages_to_send.at(i)->error) *(args->local_messages_to_send.at(i)->error)=true;
      
    }
    for(size_t i=0; i < args->local_messages_to_send.size(); i++){
      args->local_messages_to_send.at(i)->messages.clear();
      args->message_pool->Add(args->local_messages_to_send.at(i));
    }
    args->local_messages_to_send.clear();
    
  }
  //////////////////////////////////////////////////////////////////////////////////////////////
  
  return;
  
}


uint32_t SocketManager::Update(std::string ServiceName, std::string port, std::string port_name){

  if( sock == 0) return 0;
  
  paused = true;
  std::lock_guard<std::mutex> lock(sock_mtx);
  uint32_t ret = m_util->UpdateConnections(ServiceName, sock, connections, port, port_name);
  paused = false;
  
  return ret;
  
  
}
 

void SocketManager::Close(){


  if(sock != 0){
    paused = true;
    m_util->KillThread(&args);
    
    for(std::map<std::string,Store*>::iterator it = connections.begin(); it!=connections.end(); it++){
      delete it->second;
      it->second = 0;
    }
    connections.clear();
  	     
    sock = 0;

    delete m_util;
    m_util = 0;
    
  }


  
}

void SocketManager::GetStats(Store* store, std::string prefix, std::mutex* mtx){

  //  num_connections = socket_manager.connections.size();

  std::unique_ptr<std::lock_guard<std::mutex> >lock;

  seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - last).count();
  
  messages_received_rate = (num_messages_received - last_num_messages_received)/seconds; 
  replies_sent_rate = (num_replies_sent - last_num_replies_sent)/seconds; 
  receive_errors_rate = (num_receive_errors - last_num_receive_errors)/seconds;
  send_errors_rate = (num_send_errors - last_num_send_errors)/seconds;
  last_num_messages_received = num_messages_received; ///< counter for messages received
  last_num_replies_sent = num_replies_sent; ///< counter for replies sent
  last_num_receive_errors = num_receive_errors;
  last_num_send_errors = num_send_errors;
    
  if(mtx!=0)  lock.reset(new std::lock_guard<std::mutex>(*mtx));
  store->Set(prefix+"_connected", connections.size());
  store->Set(prefix+"received", last_num_messages_received);
  store->Set(prefix+"sent", last_num_replies_sent);
  store->Set(prefix+"received_errors", last_num_receive_errors);
  store->Set(prefix+"sent_errors", last_num_send_errors);
  store->Set(prefix+"received_rate", messages_received_rate);
  store->Set(prefix+"sent_rate", replies_sent_rate);
  store->Set(prefix+"received_errors_rate", receive_errors_rate);
  store->Set(prefix+"sent_errors_rate", send_errors_rate);
  
  last = std::chrono::steady_clock::now();
  
  
}

std::string SocketManager::GetConnections(){

  std::string ret="[";
  
  for(std::map<std::string,Store*>::iterator it=connections.begin(); it!= connections.end(); it++){
    ret+= it->first + " ";
  }
  ret+="]";
  
  return ret;
  
}

std::string SocketManager::SCResetConnections(const char* payload){

  ResetConnections();
    
  return "Connections Reset";

}

void SocketManager::ResetConnections(){

  paused = true;
  std::lock_guard<std::mutex> lock(sock_mtx);

  for(std::map<std::string,Store*>::iterator it=connections.begin(); it!= connections.end(); it++){
    delete it->second;
  }
  connections.clear();
 
  paused = false;

}
