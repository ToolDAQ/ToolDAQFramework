#include <SocketManager.h>

using namespace ToolFramework;

SocketManager_args::SocketManager_args():Thread_args(){}

SocketManager_args::~SocketManager_args(){

}

SocketManager::SocketManager(){}

SocketManager::~SocketManager(){

  Close();
  

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
	  args->bad_messages->Add(args->local_received_messages);
	}
	args->local_received_messages = 0;
	*(args->m_data->Log)<<"INFO: Socket Manager received bad number of message parts"<<std::endl;
	//	args->m_data->services->SendLog("Info: received bad data", LogLevel::Message);
	(*args->num_receive_errors)++;
      }
      else{
	args->received_messages->Add(args->local_received_messages);
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
    try{
      for(size_t i=0; i < args->local_messages_to_send.size(); i++){
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
	}
	else{
	  (*args->num_replies_sent)++;
	  *(args->local_messages_to_send.at(i)->sent)=true;
	}
	sock_lock.unlock();
      }
    }
    catch(...){
      *(args->m_data->Log)<<"INFO: Socket Manager caught error in send"<<std::endl;
      if(sock_lock.owns_lock()) sock_lock.unlock();
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
