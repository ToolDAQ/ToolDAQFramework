#include "MyToolZMQMultiThread.h"

MyToolZMQMultiThread_args::MyToolZMQMultiThread_args():DAQThread_args(){}

MyToolZMQMultiThread_args::~MyToolZMQMultiThread_args(){}


MyToolZMQMultiThread::MyToolZMQMultiThread():Tool(){}


bool MyToolZMQMultiThread::Initialise(std::string configfile, DataModel &data){

  InitialiseTool(data);
  InitialiseConfiguration(configfile);

  //m_variables.Print();

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

  int threadcount=0;
  if(!m_variables.Get("Threads",threadcount)) threadcount=4;

  m_util=new DAQUtilities(m_data->context);

  ManagerSend=new zmq::socket_t(*m_data->context,ZMQ_PUSH);
  ManagerSend->bind("inproc://MyToolZMQMultiThreadSend");
  ManagerReceive=new zmq::socket_t(*m_data->context,ZMQ_PULL);
  ManagerReceive->bind("inproc://MyToolZMQMultiThreadReceive");

  items[0].socket=*ManagerSend;
  items[0].fd=0;
  items[0].events=ZMQ_POLLOUT;
  items[0].revents=0;
  items[1].socket=*ManagerReceive;
  items[1].fd=0;
  items[1].events=ZMQ_POLLIN;
  items[1].revents=0;
  
  for(int i=0;i<threadcount;i++){

    MyToolZMQMultiThread_args* tmparg=new MyToolZMQMultiThread_args();   

    tmparg->ThreadReceive=new  zmq::socket_t(*m_data->context,ZMQ_PULL); 
    tmparg->ThreadReceive->connect("inproc://MyToolZMQMultiThreadSend");
    tmparg->ThreadSend=new  zmq::socket_t(*m_data->context,ZMQ_PUSH);
    tmparg->ThreadSend->connect("inproc://MyToolZMQMultiThreadReceive");
    tmparg->initems[0].socket=*(tmparg->ThreadReceive);
    tmparg->initems[0].fd=0;
    tmparg->initems[0].events=ZMQ_POLLIN;
    tmparg->initems[0].revents=0;
    tmparg->outitems[0].socket=*(tmparg->ThreadSend);
    tmparg->outitems[0].fd=0;
    tmparg->outitems[0].events=ZMQ_POLLOUT;
    tmparg->outitems[0].revents=0;

    args.push_back(tmparg);
    std::stringstream tmp;
    tmp<<"T"<<i; 
    m_util->CreateThread(tmp.str(), &Thread, args.at(i));
  }
  
  m_freethreads=threadcount;
  
  ExportConfiguration();
  
  return true;
}


bool MyToolZMQMultiThread::Execute(){

  zmq::poll(&items[0], 2, 0);

  if ((items[1].revents & ZMQ_POLLIN)){

    zmq::message_t message;
    ManagerReceive->recv(&message);
    std::istringstream iss(static_cast<char*>(message.data()));
    *m_log<<"reply = "<<iss.str()<<std::endl;
    m_freethreads++;

  }

  if ((items[0].revents & ZMQ_POLLOUT)){

    if(m_freethreads>0){

      //     Utilities::SendPointer(ManagerSend,pointer);
      std::string greeting="HI";
      zmq::message_t message(greeting.length()+1);
      snprintf ((char *) message.data(), greeting.length()+1 , "%s" , greeting.c_str()) ;
      ManagerSend->send(message);
      m_freethreads--;
      std::cout<<"sending Hi"<<std::endl;
    }

  }

  *m_log<<ML(1)<<"free threads="<<m_freethreads<<":"<<args.size()<<std::endl;
  MLC();

  // sleep(1);  for single tool testing  
  
  return true;
}


bool MyToolZMQMultiThread::Finalise(){

  for(unsigned int i=0;i<args.size();i++) {

    m_util->KillThread(args.at(i));
    delete args.at(i)->ThreadSend;
    args.at(i)->ThreadSend=0;
    delete args.at(i)->ThreadReceive;
    args.at(i)->ThreadReceive=0;

  }

  args.clear();
  
  delete m_util;
  m_util=0;

  delete ManagerSend;
  ManagerSend=0;

  delete ManagerReceive;
  ManagerReceive=0;

  return true;
}

void MyToolZMQMultiThread::Thread(Thread_args* arg){

  MyToolZMQMultiThread_args* args=reinterpret_cast<MyToolZMQMultiThread_args*>(arg);
  
  zmq::poll(&(args->initems[0]), 1, 100);

  if ((args->initems[0].revents & ZMQ_POLLIN)){
  
    zmq::message_t message;
    args->ThreadReceive->recv(&message);
    std::istringstream iss(static_cast<char*>(message.data()));
  
    sleep(10);

    zmq::poll(&(args->outitems[0]), 1, 10000);
    if (args->outitems[0].revents & ZMQ_POLLOUT){
      
      std::string greeting="hello";
      zmq::message_t message(greeting.length()+1);
      snprintf ((char *) message.data(), greeting.length()+1 , "%s" , greeting.c_str()) ;
      args->ThreadSend->send(message);
    }
    
  }

}
