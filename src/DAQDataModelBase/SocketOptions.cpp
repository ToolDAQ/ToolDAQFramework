#include <SocketOptions.h>

using namespace ToolFramework;

SocketOptions::SocketOptions(){;}

SocketOptions::~SocketOptions(){;}

bool SocketOptions::SetOptions(zmq::socket_t* sock){


  
  sock->setsockopt(ZMQ_RCVHWM, receive_high_watermark);
  sock->setsockopt(ZMQ_LINGER, linger_ms);
  sock->setsockopt(ZMQ_BACKLOG, backlog);
  sock->setsockopt(ZMQ_RCVTIMEO, receive_timeout_ms);
  sock->setsockopt(ZMQ_SNDTIMEO, send_timeout_ms);
  sock->setsockopt(ZMQ_IMMEDIATE, immediate);
  sock->setsockopt(ZMQ_ROUTER_MANDATORY,router_mandatory);
  sock->setsockopt(ZMQ_TCP_KEEPALIVE, tcp_keepalive);
  sock->setsockopt(ZMQ_TCP_KEEPALIVE_IDLE, tcp_keepalive_idle_sec);
  sock->setsockopt(ZMQ_TCP_KEEPALIVE_CNT, tcp_keepalive_count);
  sock->setsockopt(ZMQ_TCP_KEEPALIVE_INTVL, tcp_keepalive_interval_sec);
  
  return true;  

}


bool SocketOptions::LoadVariables(Store* store, std::string prefix){

  
  if(!store->Get(prefix+"ZMQ_RCVHWM", receive_high_watermark)) return false;
  if(!store->Get(prefix+"ZMQ_SNDHWM", receive_high_watermark)) return false;
  if(!store->Get(prefix+"ZMQ_LINGER", linger_ms)) return false;
  if(!store->Get(prefix+"ZMQ_BACKLOG", backlog)) return false;
  if(!store->Get(prefix+"ZMQ_RCVTIMEO", receive_timeout_ms)) return false;
  if(!store->Get(prefix+"ZMQ_SNDTIMEO", send_timeout_ms)) return false;
  if(!store->Get(prefix+"ZMQ_IMMEDIATE", immediate)) return false;
  if(!store->Get(prefix+"ZMQ_ROUTER_MANDATORY", router_mandatory)) return false;
  if(!store->Get(prefix+"ZMQ_TCP_KEEPALIVE", tcp_keepalive)) return false;
  if(!store->Get(prefix+"ZMQ_TCP_KEEPALIVE_IDLE", tcp_keepalive_idle_sec)) return false;
  if(!store->Get(prefix+"ZMQ_TCP_KEEPALIVE_CNT", tcp_keepalive_count)) return false;
  if(!store->Get(prefix+"ZMQ_TCP_KEEPALIVE_INTVL", tcp_keepalive_interval_sec)) return false;

  return true;

}
