#include <SlowControlCollection.h>

using namespace ToolFramework;

SlowControlCollectionThread_args::SlowControlCollectionThread_args(){
  
  sock=0;
  SCC=0;
  poll_length=100;
  sub=0;
  alert_functions=0;
  alert_functions_mutex=0;
}

SlowControlCollectionThread_args::~SlowControlCollectionThread_args(){
  
  delete sock;
  sock=0;
  
  delete sub;
  sub=0;
  
  SCC=0;
  alert_functions=0;
  alert_functions_mutex=0;
  
}

SlowControlCollection::SlowControlCollection(){
  
  args=0;
  m_util=0;
  m_context=0;
  m_pub=0;
  m_new_service=false;
  m_thread=false;
  
}

SlowControlCollection::~SlowControlCollection(){

  Stop();  
  
}

void SlowControlCollection::Stop(){
  //printf("p0\n");
  //  std::cout<<args<<std::endl;
  //printf("p1 %p \n",args);
if(m_thread && args) m_util->KillThread(args);
 m_thread=false;
   //printf("p2\n");
  delete args;
  args=0;
   //printf("p3\n");
  delete m_pub;
  m_pub=0;
 //printf("p4\n");
  if(m_new_service) m_util->RemoveService("SlowControlReceiver");
  m_new_service=false;
   //printf("p5\n");
  delete m_util;
  m_util=0;
   //printf("p6\n");
  Clear();
   //printf("p7\n");
}

bool SlowControlCollection::Init(zmq::context_t* context, int port, bool new_service){
  
  if(args) return false;
  //printf("Init\n");
  m_context=context;
  
  m_util=new DAQUtilities(m_context);
  m_new_service=new_service;
  
  m_pub= new zmq::socket_t(*(m_context), ZMQ_PUB);
  m_pub->setsockopt(ZMQ_LINGER, 0);
  m_pub->bind("tcp://*:78787");
  
  args=new SlowControlCollectionThread_args();
  //printf("init p=%p\n", args);
  
  args->alert_functions=&m_alert_functions;
  args->alert_functions_mutex=&m_alert_functions_mutex;
  
  args->sock = new zmq::socket_t(*(m_context), ZMQ_ROUTER);
  args->sock->setsockopt(ZMQ_LINGER, 0);
  
  std::stringstream tmp;
  tmp<<"tcp://*:"<<port;
  
  args->sock->bind(tmp.str().c_str());
  
  args->sub = new zmq::socket_t(*(m_context), ZMQ_SUB);
  args->sub->setsockopt(ZMQ_SUBSCRIBE, "", 0);
  args->sub->setsockopt(ZMQ_LINGER, 0);
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
   Add("?",SlowControlElementType(BUTTON));
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
  //printf("InitThreadedReceiver\n");
  m_thread=true;

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
    int ok = args->sock->recv(&identity);

    if(ok==0 || !identity.more()){
      std::cerr<<"error: Poorly formatted slowcontrol input [identity problem]"<<std::endl;
      return;
    }
    
    zmq::message_t blank;
    ok = args->sock->recv(&blank);
    
    if (!blank.more()){
      std::cerr<<"error: Poorly formatted slowcontrol input [blank problem]"<<std::endl;
      return;
    }
    
    zmq::message_t message;
    ok = args->sock->recv(&message);
    
    if(ok==0 || message.more()){
      std::cerr<<"error: Poorly formatted slowcontrol input [message problem]"<<std::endl;
      return;
    }
    
    std::istringstream iss(static_cast<char*>(message.data()));
    Store tmp;
    //printf("iss=%s\n",iss.str().c_str());
    tmp.JsonParser(iss.str());
    
    if(!tmp.Has("msg_value")){
      std::cerr<<"error: Poorly formatted slowcontrol input [no msg_value]"<<std::endl;
      return;
    }
    
    std::string key=tmp.Get<std::string>("msg_value");
    std::string value="";
    tmp.Get("var1",value);
    
    // std::stringstream tmpstream(str);
    // tmpstream>>str;
    
    //tmp.Print();
    //printf("key=%s\n",key.c_str());
    
    std::string reply="error: " + key;
    bool strip=false;
    
    if(key == "?"){
      //printf("args->SCC->Print()=%s\n", args->SCC->Print().c_str());
      if(value=="JSON"){
	reply=args->SCC->PrintJSON();
	strip=true;
      }
      else  reply=args->SCC->Print();
      //printf("reply=%s\n", reply.c_str());
    }
    else if((*args->SCC)[key]){
      //std::cout<<"variable exists"<<std::endl;
      if((*args->SCC)[key]->GetType() == SlowControlElementType(INFO)){
        (*args->SCC)[key]->GetValue(value);
        reply=value;
      }
      else if((*args->SCC)[key]->GetType() == SlowControlElementType(BUTTON)){
	(*args->SCC)[key]->SetValue("1");
	reply=key;
      }
      else{
	reply=key;
        //std::stringstream input;
        //input<<tmp.Get<std::string>("msg_value");
	//        std::string key="";
	// std::string value="";
        //input>>key>>value;
	//printf("d0 %s = %s : %s\n", reply.c_str(), key.c_str(), value.c_str());
        if((*args->SCC)[key]){
	  if(value!=""){
	    (*args->SCC)[key]->SetValue(value);
	    SCFunction tmp_func= (*args->SCC)[key]->GetChangeFunction();
	    if (tmp_func!=nullptr) reply=tmp_func(key.c_str());
	    
	  }
	  else{
	    SCFunction tmp_func= (*args->SCC)[key]->GetReadFunction();
	    if (tmp_func!=nullptr) reply=tmp_func(key.c_str());
	    else (*args->SCC)[key]->GetValue(reply);
	    
	  }
        }
      }
    }
    
    Store rr;
    
    rr.Set("msg_type", "Command Reply");
    rr.Set("msg_value",reply);
    if(strip) rr.Destring("msg_value");
       
    std::string tmp2="";
    rr>>tmp2;
    //printf("reply message is= %s \n",tmp2.c_str());
    zmq::message_t send(tmp2.length()+1);
    snprintf ((char *) send.data(), tmp2.length()+1 , "%s" ,tmp2.c_str()) ;
    
    
    bool tmp_ok = args->sock->send(identity, ZMQ_SNDMORE);
    if(tmp_ok) tmp_ok = tmp_ok && args->sock->send(blank, ZMQ_SNDMORE);
    if(tmp_ok) tmp_ok= tmp_ok && args->sock->send(send);
    if(!tmp_ok){
      std::cerr<<"failed to send '"<<reply<<"' to '"<<key<<"'"<<std::endl;
      return;
    }
    // FIXME these sorts of errors should be logged somewhere
    // rather than being silently ignored. This info could be critical for debugging issues!!!
  }
  
  if (args->items[1].revents & ZMQ_POLLIN){ //received alert value;
    
    zmq::message_t message;
    
    // receive alert type
    int ok = args->sub->recv(&message);
    if(ok==0){
      // FIXME this case should be handled! what do we do?
      std::cerr<<"failed to receive alert!"<<std::endl;
    }
    std::istringstream iss(static_cast<char*>(message.data()));
    
    // receive alert payload
    std::string payload;
    bool has_data=false;
    if(message.more()){
      ok = args->sub->recv(&message);
      if(ok==0){
        // FIXME this case should be handled! what do we do?
        std::cerr<<"failed to receive alert payload!"<<std::endl;
      }
      payload.resize(message.size(),'\0');
      memcpy((void*)payload.data(),message.data(),message.size());
      has_data=true;
    }
    
    //std::cout<<iss.str()<<std::endl;
    args->alert_functions_mutex->lock();
    if(args->alert_functions->count(iss.str())){
      if(has_data) (*(args->alert_functions))[iss.str()](iss.str().c_str(), payload.c_str());
      else         (*(args->alert_functions))[iss.str()](iss.str().c_str(), 0);
    }
    args->alert_functions_mutex->unlock();
    
    //if(payload!=nullptr) free(payload);
  }
  
}


void SlowControlCollection::Clear(){
  
  for(std::map<std::string, SlowControlElement*>::iterator it=SC_vars.begin(); it!=SC_vars.end(); it++){
    delete it->second;
    it->second=0;
  }
  
  SC_vars.clear();
  
}


bool SlowControlCollection::Add(std::string name, SlowControlElementType type, SCFunction change_function, SCFunction read_function){
  
  if(SC_vars.count(name)) return false;
  SC_vars[name] = new SlowControlElement(name, type, change_function, read_function);
  
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
  
  std::string reply = "";
  bool first=true;
  for(std::map<std::string, SlowControlElement*>::iterator it=SC_vars.begin(); it!=SC_vars.end(); it++){
    if(!first) reply+=", ";
    reply += it->first;
    first=false;
  }
  
  return reply;
  
}

std::string SlowControlCollection::PrintJSON(){
  
  std::string reply = "[";
  bool first=true;
  for(std::map<std::string, SlowControlElement*>::iterator it=SC_vars.begin(); it!=SC_vars.end(); it++){
    if(!first) reply+=", ";
    reply +=it->second->Print();
    first = false;
  }
  reply+="]";

  return reply;
  
}

bool SlowControlCollection::AlertSubscribe(std::string alert, AlertFunction function){
  
  if(function==nullptr) return false;
  m_alert_functions_mutex.lock();
  m_alert_functions[alert]=function;
  m_alert_functions_mutex.unlock();
  
  return true;
}


bool SlowControlCollection::AlertSend(std::string alert, std::string payload){
  
  zmq::message_t message(alert.length()+1);
  snprintf((char*) message.data(), alert.length()+1, "%s", alert.c_str());
  if(payload==""){
    return m_pub->send(message);
  }
  // if we didn't return, we have a payload as well
  m_pub->send(message, ZMQ_SNDMORE);
  zmq::message_t message2(payload.length()+1);
  snprintf((char*) message2.data(), payload.length()+1, "%s", payload.c_str());
  return m_pub->send(message2);
  
}

void SlowControlCollection::JsonParser(std::string json){

  std::map<std::string, std::string> out;

  Unpack(json, out);

  //std::cout<<"out map"<<std::endl;
  
  for(std::map<std::string, std::string>::iterator it=out.begin(); it!=out.end(); it++){

    //std::cout<<it->first<<" -> "<<it->second<<std::endl;
    
    Add(it->first, SlowControlElementType::VARIABLE);
    SC_vars[it->first]->JsonParser(it->second);
    
  }
  
  
}

void SlowControlCollection::Unpack(std::string in, std::map<std::string, std::string> &out, std::string header){

  if(header!="") header+="::";
  //std::cout<<std::endl<<std::endl<<std::endl<<"unpacking "<<header<<std::endl;//<<" : in="<<in<<std::endl;
  //std::cout<<"printing store"<<std::endl;
  Store test;
  test.JsonParser(in);
  //test.Print();
  //std::cout<<"#############################"<<std::endl<<std::endl;
  
  for(std::map<std::string, std::string>::iterator it=test.begin(); it!=test.end(); it++){
    //std::cout<<"it->first="<<it->first<<" : it->second[0]="<<it->second[0]<<" : "<<it->second<<std::endl;
    if(it->first=="SCObject"){

      Store tmp;
      tmp.JsonParser(it->second);
      out[header+tmp.Get<std::string>("name")]=it->second;
    }
    else if(it->first=="SCArray"){
      
      //      unsigned int counter=0;
      unsigned int first=0;
      for(unsigned int i=1; i<it->second.length(); i++){
	if(it->second[i]=='{') first=i;
	if(it->second[i]=='}'){
	  //  std::stringstream tmp;
	  //tmp<<counter;
	  Store tmp;
	  tmp.JsonParser(it->second.substr(first,i-first+1));
	  out[header+tmp.Get<std::string>("name")]=it->second.substr(first,i-first+1);	  
	  //	  counter++;
	}
	
      }
      
    }
    else if(it->second[0]=='{') Unpack(it->second, out, header+it->first);
    else if(it->second[0]=='['){

      int bracket_counter=0;
      unsigned int start=0;
      unsigned int counter=0;
      
      for(unsigned int i = 1; i<it->second.length(); i++){
	//std::cout<<"i="<<i<<" : it->second[i]="<<it->second[i]<<" : counter="<<counter<<" : bracket_counter="<<bracket_counter<<std::endl; 
	if(it->second[i]=='{' && bracket_counter==0) start=i;
	if(it->second[i]=='{') bracket_counter++;
	else if(it->second[i]=='}' && bracket_counter==1) {
	  std::stringstream tmp;
	  tmp<<counter;
	  Unpack(it->second.substr(start,i-start+1), out, header+it->first+"::"+tmp.str());
	  counter++;
	  bracket_counter=0;
	}
	else if (it->second[i]=='}') bracket_counter--;
      }
    }
  }

}
