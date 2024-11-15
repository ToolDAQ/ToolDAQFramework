#ifndef ServicesBackend_H
#define ServicesBackend_H

#include "Store.h"
#include "DAQUtilities.h"

#include "zmq.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

#include <string>
#include <iostream>
#include <stdio.h>       // fwrite
#include <map>
#include <queue>
#include <future>
#include <mutex>
#include <unistd.h>      // gethostname
#include <locale>        // toupper/tolower
#include <functional>    // std::function, std::negate
#include <sys/socket.h>  // multicast
#include <sys/types.h>   // multicast
#include <arpa/inet.h>   // multicast
#include <netinet/in.h>  // multicast
#include <fcntl.h>       // multicast
#include <errno.h>

namespace ToolFramework {

struct Command {
	Command(std::string command_in, char cmd_type_in, std::string topic_in);
	
	Command(const Command& cmd_in);  // copy constructor
	Command(Command&& cmd_in);       // move constructor
	Command& operator=(Command&& command_in); // move assignment operator
	Command();
	void Print() const;
	std::string command;
	std::vector<std::string> response;
	char type;
	std::string topic;
	uint32_t success;            // middleman treats this as BOOL: 1=success, 0=fail
	std::string err;
	uint32_t msg_id;
};


enum class SlowControlMsg { Query, Log, Alarm, Monitoring, Calibration, Config };

class ServicesBackend {
	public:
	ServicesBackend(){};
	~ServicesBackend(){};
	void SetUp(zmq::context_t* in_context, std::function<void(std::string msg, int msg_verb, int verbosity)> log=nullptr); // possibly move to constructor
	bool Ready(int timeout); // check if zmq sockets have connections
	bool Initialise(std::string configfile);
	bool Initialise(Store &varaibles_in);
	bool Finalise();
	
	// interfaces called by clients. These return within timeout.
	bool SendCommand(const std::string& topic, const std::string& command, std::vector<std::string>* results=nullptr, const unsigned int* timeout_ms=nullptr, std::string* err=nullptr);
	bool SendCommand(const std::string& topic, const std::string& command, std::string* results=nullptr, const unsigned int* timeout_ms=nullptr, std::string* err=nullptr);
	
	// multicasts
	bool SendMulticast(std::string command, std::string* err=nullptr);
	
	
	private:
	
	int clt_pub_port;
	int clt_dlr_port;
	mutable std::mutex send_queue_mutex;
	mutable std::mutex dlr_socket_mutex;
	
	std::function<void(std::string msg, int msg_verb, int verbosity)> m_log = nullptr;
	Store m_variables;
	DAQUtilities* utilities = nullptr;
	// required by the Utilities class to keep track of connections to clients
	std::map<std::string,Store*> clt_pub_connections;
	std::map<std::string,Store*> clt_dlr_connections;
	
	zmq::context_t* context = nullptr;
	
	zmq::socket_t* clt_pub_socket = nullptr;
	zmq::socket_t* clt_dlr_socket = nullptr;
	
	std::vector<zmq::pollitem_t> in_polls;
	std::vector<zmq::pollitem_t> out_polls;
	
	// multicast socket file descriptor
	int multicast_socket=-1;
	// multicast destination address structure
	struct sockaddr_in multicast_addr;
	socklen_t multicast_addrlen;
	// apparently works with zmq poller?
	zmq::pollitem_t multicast_poller;
	
	std::queue<std::pair<Command, std::promise<int>>> waiting_senders;
	std::map<uint32_t, std::promise<Command>> waiting_recipients;
	
	void Log(std::string msg, int msg_verb, int verbosity); //??  generalise private
        bool InitZMQ(); //private
	bool InitMulticast(); // private
	bool RegisterServices(); //private
	// wrapper funtion; add command to outgoing queue, receive response. ~30s timeout.
	bool DoCommand(Command& cmd, int timeout_ms); //private
	// actual send/receive functions
	bool SendNextCommand(); //private
	bool GetNextResponse(); //priavte
	
	bool BackgroundThread(std::future<void> terminator);
	std::thread background_thread;   // a thread that will perform zmq socket operations in the background
	std::promise<void> terminator;   // call set_value to signal the background_thread should terminate
	
	// TODO add retrying
	int max_retries;
	int inpoll_timeout;
	int outpoll_timeout;
	int command_timeout;
	
	// TODO add stats reporting
	boost::posix_time::time_duration resend_period;      // time between resends if not acknowledged
	boost::posix_time::time_duration print_stats_period; // time between printing info about what we're doing
	boost::posix_time::ptime last_write;                 // when we last sent a write command
	boost::posix_time::ptime last_read;                  // when we last sent a read command
	boost::posix_time::ptime last_printout;              // when we last printed out stats about what we're doing
	
	int read_commands_failed;
	int write_commands_failed;
	
	// general
	int verbosity;
	// verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	int get_ok;
	boost::posix_time::time_duration elapsed_time;
	std::string hostname;   // for printing with stats
	int execute_iterations=0;
	
	// ZMQ socket identities - we really only need the reply socket one
	// since that's the one the middleman needs to know to send replies back
	std::string clt_ID;
	
	uint32_t msg_id = 0;
	
	// =======================================================
	
	// zmq helper functions
	// TODO move to separate class as these are shared by middleman
	
	int PollAndReceive(zmq::socket_t* sock, zmq::pollitem_t poll, int timeout, std::vector<zmq::message_t>& outputs);
	bool Receive(zmq::socket_t* sock, std::vector<zmq::message_t>& outputs);
	
	// base cases; send single (final) message part
	// 1. case where we're given a zmq::message_t -> just send it
	bool Send(zmq::socket_t* sock, bool more, zmq::message_t& message);
	// 2. case where we're given a std::string -> specialise accessing underlying data
	bool Send(zmq::socket_t* sock, bool more, std::string messagedata);
	// 3. case where we're given a vector of strings
	bool Send(zmq::socket_t* sock, bool more, std::vector<std::string> messages);
	// 4. generic case for other primitive types
	template <typename T>
	typename std::enable_if<std::is_fundamental<T>::value, bool>::type
	Send(zmq::socket_t* sock, bool more, T messagedata){
		if(verbosity>10) std::cout<<__PRETTY_FUNCTION__<<" called"<<std::endl;
		zmq::message_t message(sizeof(T));
		memcpy(message.data(), &messagedata, sizeof(T));
		bool send_ok;
		if(more){
			send_ok = sock->send(message, ZMQ_SNDMORE);
			if(verbosity>10) std::cout<<"zmq sent next part: "<<send_ok<<std::endl;
		} else {
			send_ok = sock->send(message);
			if(verbosity>10) std::cout<<"zmq sent final part: "<<send_ok<<std::endl;
		}
		if(verbosity>10) std::cout<<"returning "<<send_ok<<std::endl;
		return send_ok;
	}
	
	// recursive case; send the next message part and forward all remaining parts
	template <typename T1, typename T2, typename... Rest>
	bool Send(zmq::socket_t* sock, bool more, T1&& msg1, T2&& msg2, Rest&&... rest){
		if(verbosity>10) std::cout<<__PRETTY_FUNCTION__<<" called"<<std::endl;
		bool send_ok = Send(sock, true, std::forward<T1>(msg1));
		if(not send_ok) return false;
		send_ok = Send(sock, more, std::forward<T2>(msg2), std::forward<Rest>(rest)...);
		if(verbosity>10) std::cout<<"returning "<<send_ok<<std::endl;
		return send_ok;
	}
	
	// wrapper to do polling if required
	// version if one part
	template <typename T>
	int PollAndSend(zmq::socket_t* sock, zmq::pollitem_t poll, int timeout, T&& message){
		if(verbosity>10) std::cout<<__PRETTY_FUNCTION__<<" called"<<std::endl;
		int send_ok=0;
		// check for listener
		int ret=0;
		try {
			ret = zmq::poll(&poll, 1, timeout);
		} catch (zmq::error_t& err){
			std::cerr<<"ServicesBackend::PollAndSend poller caught "<<err.what()<<std::endl;
			ret = -1;
		}
		if(ret<0){
			// error polling - is the socket closed?
			send_ok = -3;
		} else if(poll.revents & ZMQ_POLLOUT){
			bool success = Send(sock, false, std::forward<T>(message));
			send_ok = success ? 0 : -1;
		} else {
			// no listener
			send_ok = -2;
		}
		if(verbosity>10) std::cout<<"returning "<<send_ok<<std::endl;
		return send_ok;
	}
	
	// wrapper to do polling if required
	// version if more than one part
	template <typename T, typename... Rest>
	int PollAndSend(zmq::socket_t* sock, zmq::pollitem_t poll, int timeout, T&& message, Rest&&... rest){
		if(verbosity>10) std::cout<<__PRETTY_FUNCTION__<<" called"<<std::endl;
		int send_ok = 0;
		// check for listener
		int ret = 0;
		try {
			ret = zmq::poll(&poll, 1, timeout);
		} catch (zmq::error_t& err){
			std::cerr<<"ServicesBackend::PollAndSend poller caught "<<err.what()<<std::endl;
			ret = -1;
		}
		if(ret<0){
			// error polling - is the socket closed?
			send_ok = -3;
		} else if(poll.revents & ZMQ_POLLOUT){
			bool success = Send(sock, false, std::forward<T>(message), std::forward<Rest>(rest)...);
			send_ok = success ? 0 : -1;
		} else {
			// no listener
			send_ok = -2;
		}
		if(verbosity>10) std::cout<<"returning "<<send_ok<<std::endl;
		return send_ok;
	}
	
};

}

#endif
