#include <SlowControlCollection.h>

SlowControlCollectionThread_args::SlowControlCollectionThread_args(){

  sock=0;
  SCC=0;
  poll_length=100;
  sub=0;
  trigger_functions=0;
  trigger_functions_mutex=0;
}

SlowControlCollectionThread_args::~SlowControlCollectionThread_args(){

  delete sock;
  sock=0;

  delete sub;
  sub=0;

  SCC=0;
  trigger_functions=0;
  trigger_functions_mutex=0;

}

SlowControlCollection::SlowControlCollection(){
  
  args=0;
  m_util=0;
  m_context=0;
  m_pub=0;

}

SlowControlCollection::~SlowControlCollection(){

  if(args) m_util->KillThread(args);

  delete args;
  args=0;

  delete m_pub;
  m_pub=0;

  delete m_util;
  m_util=0;

  Clear();

}

bool SlowControlCollection::Init(zmq::context_t* context, int port, bool new_service){
  
  if(args) return false;
  
  m_context=context;
  
  m_util=new DAQUtilities(m_context);
  
  m_pub= new zmq::socket_t(*(m_context), ZMQ_PUB);
  m_pub->bind("tcp://*:78787");

  args=new SlowControlCollectionThread_args();
  
  args->trigger_functions=&m_trigger_functions;
  args->trigger_functions_mutex=&m_trigger_functions_mutex;
  
  args->sock = new zmq::socket_t(*(m_context), ZMQ_ROUTER);
  
  std::stringstream tmp;
  tmp<<"tcp://*:"<<port;
  
  args->sock->bind(tmp.str().c_str());

  args->sub = new zmq::socket_t(*(m_context), ZMQ_SUB);
  args->sub->setsockopt(ZMQ_SUBSCRIBE, "", 0);
  args->sub->bind("tcp://*:78788");
  
  
  //  std::cout<<"new_service="<<new_service<<std::endl;
  if(new_service && !m_util->AddService("SlowControlReceiver",port,false)){
    
    delete args->sock;
    args->sock=0;
    
    delete args;
    args=0;
    
    return false;
  }

  args->items[0].socket=*(args->sock);
  args->items[0].fd=0;
  args->items[0].events=ZMQ_POLLIN;
  args->items[0].revents=0;
  args->items[1].socket=*(args->sub);
  args->items[1].fd=0;
  args->items[1].events=ZMQ_POLLIN;
  args->items[1].revents=0;
  
  args->SCC=this;
  
  Add("Status",SlowControlElementType(INFO));
  SC_vars["Status"]->SetValue("N/A");
  
  return true;
}

bool SlowControlCollection::ListenForData(int poll_length){

  args->poll_length=poll_length;
  Thread(args);

  return true;

}

bool SlowControlCollection::InitThreadedReceiver(zmq::context_t* context, int port, int poll_length, bool new_service){

  if(args) return false;
 
  //std::cout<<"new_service="<<new_service<<std::endl;
  Init(context, port, new_service);
  args->poll_length=poll_length;   
  m_util->CreateThread("receiver", &Thread, args);
  
  return true;
}

void SlowControlCollection::Thread(Thread_args* arg){

  SlowControlCollectionThread_args* args=reinterpret_cast<SlowControlCollectionThread_args*>(arg);

  zmq::poll(&(args->items[0]), 2, args->poll_length);

  if (args->items[0].revents & ZMQ_POLLIN){ //received slow control value

    zmq::message_t identity;
    args->sock->recv(&identity);

    zmq::message_t blank;
    args->sock->recv(&blank);

    zmq::message_t message;
    args->sock->recv(&message);
    std::istringstream iss(static_cast<char*>(message.data()));
    Store tmp;
    tmp.JsonParser(iss.str());

    std::string str="";

    tmp.Get("msg_value", str);
    //tmp.Print();
    //std::cout<<"str="<<str<<std::endl;

    std::string reply="error: " + *tmp["msg_value"];

    if(str == "?") reply=args->SCC->Print();
    else if((*args->SCC)[str]){
      if((*args->SCC)[str]->GetType() == SlowControlElementType(INFO)){
	std::string value="";
	(*args->SCC)[str]->GetValue(value);     
	reply=value;
      }
      else{
	reply=*tmp["msg_value"];
	std::stringstream input;
	input<<*tmp["msg_value"];
	std::string key="";
	std::string value="";   
	input>>key>>value;
	if(value=="") value="1";
	if((*args->SCC)[key]){
	  (*args->SCC)[key]->SetValue(value);
	  std::function<std::string(std::string)> tmp_func= (*args->SCC)[key]->GetFunction();
	  if (tmp_func!=nullptr) reply=tmp_func(key);
	  //std::cout<<"value="<<value<<std::endl;
	}
      }
    }
    
    Store rr;
    
    *rr["msg_type"]="Command Reply";
    rr.Set("msg_value",reply);
    
    std::string tmp2="";
    rr>>tmp2;
    zmq::message_t send(tmp2.length()+1);
    snprintf ((char *) send.data(), tmp2.length()+1 , "%s" ,tmp2.c_str()) ;
    
    
    args->sock->send(identity, ZMQ_SNDMORE);
    args->sock->send(blank, ZMQ_SNDMORE);
    args->sock->send(send);
  }

  if (args->items[1].revents & ZMQ_POLLIN){ //received trigger value;

    zmq::message_t message;

    args->sub->recv(&message);
    std::istringstream iss(static_cast<char*>(message.data()));
    //std::cout<<iss.str()<<std::endl;
    args->trigger_functions_mutex->lock();
    if(args->trigger_functions->count(iss.str())) (*(args->trigger_functions))[iss.str()](iss.str());
    args->trigger_functions_mutex->unlock();
  } 

}


void SlowControlCollection::Clear(){
  
  for(std::map<std::string, SlowControlElement*>::iterator it=SC_vars.begin(); it!=SC_vars.end(); it++){
    delete it->second;
    it->second=0;
  }
  
  SC_vars.clear();
  
}


bool SlowControlCollection::Add(std::string name, SlowControlElementType type, std::function<std::string(std::string)> function){

  if(SC_vars.count(name)) return false;
  SC_vars[name] = new SlowControlElement(name, type, function);
  
  return true;
  
}

bool SlowControlCollection::Remove(std::string name){

  if(SC_vars.count(name)){
    std::map<std::string, SlowControlElement*>::iterator it;
    for(it=SC_vars.begin(); it!=SC_vars.end(); it++){
      if(it->first==name) break;
  }
    delete it->second;
    it->second=0;
    SC_vars.erase(it);
    return true;  
  }

  return false;

}

SlowControlElement* SlowControlCollection::operator[](std::string key){
  if(SC_vars.count(key)) return SC_vars[key];
  return 0;  
}

std::string SlowControlCollection::Print(){

  std::string reply = "?";   
  for(std::map<std::string, SlowControlElement*>::iterator it=SC_vars.begin(); it!=SC_vars.end(); it++){
    reply += ", " + it->second->Print();
  } 
  
  return reply;
  
}

bool SlowControlCollection::TriggerSubscribe(std::string trigger, std::function<void(std::string)> function){

  if(function==nullptr) return false;
  m_trigger_functions_mutex.lock(); 
  m_trigger_functions[trigger]=function;
  m_trigger_functions_mutex.unlock();

  return true;
}


bool SlowControlCollection::TriggerSend(std::string trigger){

  zmq::message_t message(trigger.length()+1);
  snprintf((char*) message.data(), trigger.length()+1, "%s", trigger.c_str());
  m_pub->send(message);
  
  return true;
  
}
