#ifndef SOCKET_OPTIONS_H
#define SOCKET_OPTIONS_H

#include <vector>
#include <zmq.hpp>
#include <string>
#include <Store.h>

namespace ToolFramework{

  class SocketOptions{
    
  public:
    
    SocketOptions();
    ~SocketOptions(); 
    
    bool SetOptions(zmq::socket_t* sock);
    
    bool LoadVariables(Store* store, std::string prefix="");
    
    std::vector<std::string> variable_names;
    int32_t send_high_watermark = 1;
    int32_t receive_high_watermark = 20000;
    int32_t linger_ms = 100;
    int32_t backlog = 5000;
    int32_t receive_timeout_ms = 1000;
    int32_t send_timeout_ms = 200;
    int32_t immediate = 1;
    int32_t router_mandatory = 1;
    int32_t tcp_keepalive = 1;
    int32_t tcp_keepalive_idle_sec = 5;
    int32_t tcp_keepalive_count = 12;
    int32_t tcp_keepalive_interval_sec = 5;
       
  };
  
}

#endif
