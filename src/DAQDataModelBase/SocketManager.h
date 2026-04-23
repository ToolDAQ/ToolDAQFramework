#ifndef SOCKET_MANAGER_H
#define SOCKET_MANAGER_H

#include <string>
#include <iostream>
#include <atomic>
#include <mutex>
#include <vector>
#include <zmq.hpp>
#include <Buffer.h>
#include <Pool.h>
#include <DAQUtilities.h>
#include <ZMQMessages.h>
#include <DataModel.h>


namespace ToolFramework{
  

  
  struct SocketManager_args:Thread_args{
    
    SocketManager_args();
    ~SocketManager_args();
    
    DataModel* m_data;
    zmq::socket_t* sock = 0; ///< socket pointer for receiving data form electronics
    std::mutex* sock_mtx = 0;
    zmq::pollitem_t items[2]; ///< poll list to avoid blocking
    
    Pool<ZMQMessages >* message_pool;
    Buffer<ZMQMessages* >* received_messages = 0;
    ZMQMessages* local_received_messages; ///< temporary pointer for received messages
    Buffer<ZMQMessages* >* messages_to_send = 0;
    std::vector<ZMQMessages* > local_messages_to_send;
    Buffer<ZMQMessages* >* bad_messages = 0;
    uint16_t expected_number_messages = 0;
    
    std::atomic<uint64_t>* num_messages_received; ///< counter for messages received
    std::atomic<uint64_t>* num_replies_sent; ///< counter for replies sent
    std::atomic<uint64_t>* num_receive_errors;
    std::atomic<uint64_t>* num_send_errors;
    
    bool return_check = true; ///< variable for checking retuns values
    bool* paused = 0;
    int poll_return = 0;
    
  };
  
  class SocketManager{
    
  public:
    SocketManager();
    ~SocketManager();
    bool Init(DataModel* data_model, zmq::socket_t* in_sock, Buffer<ZMQMessages* >* receive_buffer, Buffer<ZMQMessages* >* send_buffer, Buffer<ZMQMessages* >* error_buffer, Pool<ZMQMessages>* message_pool, uint16_t expected_number_messages=0);
    uint32_t Update(std::string ServiceName, std::string port="", std::string port_name="");
    void Close();
 
    std::atomic<uint64_t> num_messages_received; ///< counter for messages received
    std::atomic<uint64_t> num_replies_sent; ///< counter for replies sent
    std::atomic<uint64_t> num_receive_errors;
    std::atomic<uint64_t> num_send_errors;

    std::map<std::string,Store*> connections;
    
  private:
    
    void CreateThread();
    static void Thread(Thread_args* arg);     
    
    SocketManager_args args;
    DAQUtilities* m_util = 0; 
    DataModel* m_data = 0;
    zmq::socket_t* sock = 0; 
    std::mutex sock_mtx;
    bool paused = false;
    
  };
  
}

#endif
