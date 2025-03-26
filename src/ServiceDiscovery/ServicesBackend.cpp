#include "ServicesBackend.h"

using namespace ToolFramework;

Command::Command(std::string command_in, char type_in, std::string topic_in){
	command = command_in;
	type = type_in;
	topic=topic_in;
	success=0;
	response=std::vector<std::string>{};
	err="";
	msg_id=0;
}

Command::Command(){
	command="";
	type='\0';
	topic="";
	success=0;
	response=std::vector<std::string>{};
	err="";
	msg_id=0;
}

void Command::Print() const {
	std::cout<<"command="<<command<<std::endl;
	std::cout<<"type="<<type<<std::endl;
	std::cout<<"topic="<<topic<<std::endl;
	std::cout<<"success="<<success<<std::endl;
	std::cout<<"err="<<err<<std::endl;
	std::cout<<"id="<<msg_id<<std::endl;
	std::cout<<"response.size()="<<response.size()<<std::endl;
	for(auto&& aresp : response){
		std::cout<<aresp<<std::endl;
	}
}

Command::Command(const Command& cmd_in){
	command = cmd_in.command;
	type = cmd_in.type;
	topic = cmd_in.topic;
	success = cmd_in.success;
	response = cmd_in.response;
	err = cmd_in.err;
	msg_id = cmd_in.msg_id;
}

Command::Command(Command&& cmd_in){
	command = cmd_in.command;
	type = cmd_in.type;
	topic = cmd_in.topic;
	success = cmd_in.success;
	response = std::move(cmd_in.response);
	err = cmd_in.err;
	msg_id = cmd_in.msg_id;
	cmd_in.command="";
	cmd_in.type='\0';
	cmd_in.topic="";
	cmd_in.success=0;
	cmd_in.msg_id=0;
	cmd_in.response=std::vector<std::string>{};
}

Command& Command::operator=(Command&& cmd_in){
	if(this!=&cmd_in){
		command = cmd_in.command;
		type = cmd_in.type;
		topic = cmd_in.topic;
		success = cmd_in.success;
		response = std::move(cmd_in.response);
		err = cmd_in.err;
		msg_id = cmd_in.msg_id;
		cmd_in.command="";
		cmd_in.type='\0';
		cmd_in.topic="";
		cmd_in.success=0;
		cmd_in.msg_id=0;
		cmd_in.response=std::vector<std::string>{};
	}
	return *this;
}

void ServicesBackend::SetUp(zmq::context_t* in_context, std::function<void(std::string msg, int msg_verb, int verbosity)> log){
	
	context = in_context;
	m_log = log;
	
}

bool ServicesBackend::Initialise(std::string configfile){

  /*               Retrieve Configs            */
  /* ----------------------------------------- */
  
  // configuration options can be parsed via a Store class
  if(configfile!="") m_variables.Initialise(configfile);

  return Initialise(m_variables);

}
  
bool ServicesBackend::Initialise(Store &variables_in){

  std::string tmp="";
  variables_in>>tmp;
  m_variables.JsonParser(tmp);
	
	/*            General Variables              */
	/* ----------------------------------------- */
	if(!m_variables.Get("verbosity",verbosity)) verbosity = 1;
	if(!m_variables.Get("max_retries",max_retries)) max_retries = 3;
	int advertise_endpoints = 1;
	m_variables.Get("advertise_endpoints",advertise_endpoints);
	
	get_ok = InitZMQ();
	if(not get_ok) return false;
	
	get_ok = InitMulticast();
	if(not get_ok) return false;
	
	// new HK version; don't advertise endpoints, middleman just assumes they exist
	if(advertise_endpoints){
		get_ok &= RegisterServices();
		if(not get_ok) return false;
	}
	
	/*                Time Tracking              */
	/* ----------------------------------------- */
	
	// time to wait between resend attempts if not ack'd
	int resend_period_ms = 1000;
	// how often to print out stats on what we're sending
	int print_stats_period_ms = 5000;
	
	// Update with user-specified values.
	m_variables.Get("resend_period_ms",resend_period_ms);
	m_variables.Get("print_stats_period_ms",print_stats_period_ms);
	
	// convert times to boost for easy handling
	resend_period = boost::posix_time::milliseconds(resend_period_ms);
	print_stats_period = boost::posix_time::milliseconds(print_stats_period_ms);
	
	// initialise 'last send' times
	last_write = boost::posix_time::microsec_clock::universal_time();
	last_read = boost::posix_time::microsec_clock::universal_time();
	last_printout = boost::posix_time::microsec_clock::universal_time();
	
	// get the hostname of this machine for monitoring stats
	char buf[255];
	get_ok = gethostname(buf, 255);
	if(get_ok!=0){
		std::cerr<<"Error getting hostname!"<<std::endl;
		perror("gethostname: ");
		hostname = "unknown";
	} else {
		hostname = std::string(buf);
	}
	
	// initialise the message IDs based on the current time in unix seconds
	//msg_id = (int)time(NULL); -> not unique enough
	uint64_t nanoseconds_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	msg_id = static_cast<uint32_t>(nanoseconds_since_epoch);
	if(verbosity>3) std::cout<<"initialising message ID to "<<msg_id<<std::endl;
	
	// kick off a thread to do actual send and receive of messages
	terminator = std::promise<void>{};
	std::future<void> signal = terminator.get_future();
	background_thread = std::thread(&ServicesBackend::BackgroundThread, this, std::move(signal));
	
	return true;
}

bool ServicesBackend::InitZMQ(){
	
	/*                  ZMQ Setup                */
	/* ----------------------------------------- */
	
	// we have two zmq sockets:
	// 1. [PUB]    one for sending write commands to all listeners (the master)
	// 2. [DEALER] one for sending read commands round-robin and receving responses
	
	// specify the ports everything talks/listens on
	clt_pub_port = 55556;   // for sending write commands
	clt_dlr_port = 55555;   // for sending read commands
	// socket timeouts, so nothing blocks indefinitely
	int clt_pub_socket_timeout=500;
	int clt_dlr_socket_timeout=500;  // both send and receive
	
	// poll timeouts - units are milliseconds
	inpoll_timeout=500;
	outpoll_timeout=500;
	
	// total timeout on how long we wait for response from a command
	command_timeout=2000;
	
	// Update with user-specified values.
	m_variables.Get("clt_pub_port",clt_pub_port);
	m_variables.Get("clt_dlr_port",clt_dlr_port);
	m_variables.Get("clt_pub_socket_timeout",clt_pub_socket_timeout);
	m_variables.Get("clt_dlr_socket_timeout",clt_dlr_socket_timeout);
	m_variables.Get("inpoll_timeout",inpoll_timeout);
	m_variables.Get("outpoll_timeout",outpoll_timeout);
	m_variables.Get("command_timeout",command_timeout);
	
	// to send replies the middleman must know who to send them to.
	// for read commands, the receiving router socket will append the ZMQ_IDENTITY of the sender
	// which can be given to the sending router socket to identify the recipient.
	// BUT the default ZMQ_IDENTITY of a socket is empty! We must set it ourselves to be useful!
	// for write commands we ALSO need to manually insert the ZMQ_IDENTITY into the written message,
	// because the receiving sub socket does not do this automaticaly.
	
	// using 'getsockopt(ZMQ_IDENTITY)' without setting it first produces an empty string,
	// so seems to need to set it manually to be able to know what the ID is, and
	// insert it into the write commands.
	// FIXME replace with whatever ben wants?
	clt_ID=m_variables.Get<std::string>("UUID");

	/*
	get_ok = m_variables.Get("ZMQ_IDENTITY",clt_ID);
	if(!get_ok){
		boost::uuids::uuid u = boost::uuids::random_generator()();
		clt_ID = boost::uuids::to_string(u);
		std::cerr<<"Warning! no ZMQ_IDENTITY in ServicesBackend settings!"<<std::endl;
	}
	*/
	clt_ID += '\0';
	
	
	// socket to publish write commands
	// -------------------------------
	clt_pub_socket = new zmq::socket_t(*context, ZMQ_PUB);
	clt_pub_socket->setsockopt(ZMQ_LINGER,10);
	clt_pub_socket->setsockopt(ZMQ_SNDTIMEO, clt_pub_socket_timeout);
	clt_pub_socket->bind(std::string("tcp://*:")+std::to_string(clt_pub_port));
	
	// socket to deal read commands and receive responses
	// -------------------------------------------------
	clt_dlr_socket = new zmq::socket_t(*context, ZMQ_DEALER);
	clt_dlr_socket->setsockopt(ZMQ_LINGER,10);
	clt_dlr_socket->setsockopt(ZMQ_SNDTIMEO, clt_dlr_socket_timeout);
	clt_dlr_socket->setsockopt(ZMQ_RCVTIMEO, clt_dlr_socket_timeout);
	clt_dlr_socket->setsockopt(ZMQ_IDENTITY, clt_ID.c_str(), clt_ID.length());
	clt_dlr_socket->setsockopt(ZMQ_IMMEDIATE,1);
	clt_dlr_socket->bind(std::string("tcp://*:")+std::to_string(clt_dlr_port));
	
	/*
	// debug: check socket option
	char cltidbuff[255];
	size_t len=255;
	clt_dlr_socket->getsockopt(ZMQ_IDENTITY,(void*)(&cltidbuff[0]),&len);
	std::cout<<"retrieved socket length was: "<<len<<" chars with contents:"<<std::endl;
	fwrite(cltidbuff,sizeof(cltidbuff[0]),len,stdout);
	*/
	
	// bundle the polls together so we can do all of them at once
	zmq::pollitem_t clt_pub_socket_pollout= zmq::pollitem_t{*clt_pub_socket,0,ZMQ_POLLOUT,0};
	zmq::pollitem_t clt_dlr_socket_pollin = zmq::pollitem_t{*clt_dlr_socket,0,ZMQ_POLLIN,0};
	zmq::pollitem_t clt_dlr_socket_pollout = zmq::pollitem_t{*clt_dlr_socket,0,ZMQ_POLLOUT,0};
	
	in_polls = std::vector<zmq::pollitem_t>{clt_dlr_socket_pollin};
	out_polls = std::vector<zmq::pollitem_t>{clt_pub_socket_pollout,
	                                         clt_dlr_socket_pollout};
	
	return true;
}

bool ServicesBackend::InitMulticast(){
	
	/*              Multicast Setup              */
	/* ----------------------------------------- */
	
	int multicast_port = 55554;
	std::string multicast_address = "239.192.1.1"; // FIXME suitable default?
	
	// FIXME add to config file
	m_variables.Get("multicast_port",multicast_port);
	m_variables.Get("multicast_address",multicast_address);
	
	// set up multicast socket for sending logging & monitoring data
	multicast_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(multicast_socket<=0){
		Log(std::string{"Failed to open multicast socket with error "}+strerror(errno),v_error,verbosity);
		return false;
	}
	
	// set linger options - do not linger, discard queued messages on socket close
	struct linger l;
	l.l_onoff  = 0;  // whether to linger
	l.l_linger = 0;  // seconds to linger for
	get_ok = setsockopt(multicast_socket, SOL_SOCKET, SO_LINGER,(char*) &l, sizeof(l));
	if(get_ok!=0){
		Log(std::string{"Failed to set multicast socket linger with error "}+strerror(errno),v_error,verbosity);
		return false;
	}
	
	// set the socket to non-blocking mode - seems like a good idea...? XXX
	get_ok = fcntl(multicast_socket, F_SETFL, O_NONBLOCK);
	if(get_ok!=0){
		Log(std::string{"Failed to set multicast socket to non-blocking with error "}
		   +strerror(errno),v_error,verbosity);
		//return false;
	}
	
	// format destination address from IP string
	bzero((char *)&multicast_addr, sizeof(multicast_addr)); // init to 0
	multicast_addr.sin_family = AF_INET;
	multicast_addr.sin_port = htons(multicast_port);
	// convert destination address string to binary
	get_ok = inet_aton(multicast_address.c_str(), &multicast_addr.sin_addr);
	if(get_ok==0){
		Log("Bad multicast address '"+multicast_address+"'",v_error,verbosity);
		return false;
	}
	multicast_addrlen = sizeof(multicast_addr);
	
	// apparently we can poll with zmq?
	multicast_poller = zmq::pollitem_t{ NULL, multicast_socket, ZMQ_POLLOUT, 0 };
	
	return true;
}

bool ServicesBackend::RegisterServices(){
	
	/*             Register Services             */
	/* ----------------------------------------- */
	
	// to register our command and response ports with the ServiceDiscovery class
	// we can make our lives a little easier by using a Utilities class
	utilities = new DAQUtilities(context);
	
	// we can now register the client sockets with the following:
	utilities->AddService("slowcontrol_write", clt_pub_port);
	utilities->AddService("slowcontrol_read",  clt_dlr_port);
	
	return true;
}

void ServicesBackend::Log(std::string msg, int msg_verb, int verbosity){
	// this is normally defined in Tool.h
	if(m_log) m_log(msg, msg_verb, verbosity);
	else if(msg_verb<= verbosity) std::cout<<msg<<std::endl;
	return;
}


bool ServicesBackend::BackgroundThread(std::future<void> signaller){
	
	Log("ServicesBackend BackgroundThread starting!",v_debug,verbosity);
	while(true){
		// check if we've been signalled to terminate
		std::chrono::milliseconds span(10);
		if(signaller.wait_for(span)!=std::future_status::timeout){
			// terminate has been set
			Log("ServicesBackend background thread received terminate signal",v_debug,verbosity);
			break;
		}
		
		// otherwise continue our duties
		get_ok = GetNextResponse();
		get_ok = SendNextCommand();
	}
	
	return true;
}

bool ServicesBackend::SendMulticast(std::string command, std::string* err){
	// multicast send. These do not wait for a response, so no timeout.
	// only immediately evident errors are reported. receipt is not confirmed.
	if(verbosity>10) std::cout<<"ServicesBackend::SendMulticast invoked with command '"<<command<<"'"<<std::endl;
	
	// check for listeners...?
	zmq::poll(&multicast_poller,1, 0);   // timeout 0 = return immediately... XXX
	if(multicast_poller.revents & ZMQ_POLLOUT){
		
		// got a listener - ship it
		int cnt = sendto(multicast_socket, command.c_str(), command.length()+1, 0, (struct sockaddr*)&multicast_addr, multicast_addrlen);
		if(cnt < 0){
			std::string errmsg = "Error sending multicast message: "+std::string{strerror(errno)};
			Log(errmsg,v_error,verbosity);
			if(err) *err= errmsg; //zmq_strerror(errno);
			return false;
		}
		
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// FIXME ok, i think the combination of SendCommand and DoCommand are an over-complication.             //
// SendCommand is basically a pointless wrapper that spawns a thread to do what it could do itself.     //
// This would remove the surplus promise/future layer. The only change to DoCommand required            //
// would be that the timeouts from each stage (wait for send, wait for reply)                           //
// would need to be appropriately modified -.e.g if total timeout is 300ms and it takes 100ms to send,  //
// then we would only wait for 200ms for the reply.                                                     //
//////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ServicesBackend::SendCommand(const std::string& topic, const std::string& command, std::vector<std::string>* results, const unsigned int* timeout_ms, std::string* err){
	// send a command and receive response.
	// This is a wrapper that ensures we always return within the requested timeout.
	if(verbosity>10) std::cout<<"ServicesBackend::SendCommand invoked with command '"<<command<<"'"<<std::endl;
	
	// encapsulate the command in an object.
	// We need this since we can only get one return value from an asynchronous function call,
	// and we want both a response string and error flag.
	if(verbosity>10) std::cout<<"ServicesBackend::SendCommand constructing Command"<<std::endl;
	
	// for now, different message types need to go out on different sockets:
	// writes go over pub sockets and reads over dealer sockets. (This may change soon...)
	// libDAQInterface prefixes the socket with `R_` or `W_` to indicate which kind of socket to use.
	if(topic.length()==0){
		std::string errmsg = "ServicesBackend::SendCommand received empty topic for command '"
		                   + command + "'";
		Log(errmsg,v_error,verbosity);
		if(err) *err= errmsg;
		return false;
	}
	char type = std::tolower(topic[0]);
	if(type!='r' && type!='w'){
		std::string errmsg = "ServicesBackend::SendCommand received invalid topic for command '"
		                   + command + "'; please prefix with 'r_' or 'w_' to indicate socket type to use";
		Log(errmsg,v_error,verbosity);
		if(err) *err= errmsg;
		return false;
	}
	
	// In the case of pub sockets, we may also want to specify a 'topic' that the recipient
	// can use with ZMQ_SUBSCRIBE to filter out particular messages.
	// In fact it's useful to indicate a topic in all cases, even when the actual message will
	// (for now) go over a dealer/router combination that cannot filter on the topic.
	Command cmd{command, type, topic.substr(2,std::string::npos)};
	
	// submit the command asynchrously.
	// This way we have control over how long we wait for the response
	// The response will be a Command object with remaining members populated.
	//std::future<Command> response = std::async(std::launch::async, &ServicesBackend::DoCommand, this, cmd);
	// std::async returns a std::future that will block on destruction until the promise returns.
	// if we don't want that to happen, i.e. we want to abandon it if it times out,
	// we instead need to obtain a future from a promise (which is somehow not blocking?),
	// and run our code in a detached thread, using the promise to pass back the result.
	// tbh i don't quite get why this is different but there we go.
	// see https://stackoverflow.com/a/23454840/3544936 and https://stackoverflow.com/a/23460094/3544936
	std::promise<Command> returnval;
	std::future<Command> response = returnval.get_future();
	if(verbosity>10) std::cout<<"ServicesBackend::SendCommand spinning up new thread"<<std::endl;
	std::thread{&ServicesBackend::DoCommand, this, cmd, std::move(returnval)}.detach();
	
	// the return from a std::async call is a 'future' object
	// this object will be populated with the return value when it becomes available,
	// but we can wait for a given timeout and then bail if it hasn't resolved in time.
	
	int timeout=command_timeout;            // default timeout for submission of command and receipt of response
	if(timeout_ms) timeout=*timeout_ms;     // override by user if a custom timeout is given
	std::chrono::milliseconds span(timeout);
	// wrap our attempt to get the future in a try-catch, in case of exception
	try {
		// wait_for will return either when the result is ready, or when it times out
		if(verbosity>10) std::cout<<"ServicesBackend::SendCommand waiting for response"<<std::endl;
		if(response.wait_for(span)!=std::future_status::timeout){
			// we got a response in time. retrieve and parse return value
			if(verbosity>10) std::cout<<"ServicesBackend::SendCommand fetching response"<<std::endl;
			cmd = response.get();
			if(verbosity>10) std::cout<<"ServicesBackend::SendCommand response is "<<cmd.response.size()
			                          <<" parts"<<std::endl;
			if(results) *results = cmd.response;
			if(err) *err = cmd.err;
			return cmd.success;
		} else {
			// timed out
			std::string errmsg="Timed out after waiting "+std::to_string(timeout)+"ms for response "
			                   "from command '"+command+"'";
			if(verbosity>3) std::cerr<<errmsg<<std::endl;
			if(err) *err=errmsg;
			//std::cout<<"SendCommand returning false"<<std::endl;
			return false;
		}
	} catch(std::exception& e){
		// one thing that can cause an exception is if we terminate the application
		// before the promise is fulfilled (i.e. the response came, or zmq timed out)
		// so long as we catch it's fine.
		Log(std::string{"ServicesBackend caught "}+e.what()+" waiting for command "+command,v_error,verbosity);
	}
	return false;
}

bool ServicesBackend::SendCommand(const std::string& topic, const std::string& command, std::string* results, const unsigned int* timeout_ms, std::string* err){
	// wrapper for when user expects only one returned response part
	if(err) *err="";
	std::vector<std::string> resultsvec;
	bool ret = SendCommand(topic, command, &resultsvec, timeout_ms, err);
	if(resultsvec.size()>0 && results!=nullptr) *results = resultsvec.front();
	// if more than one part returned, flag as error
	if(resultsvec.size()>1){
		*err += ". Command returned "+std::to_string(resultsvec.size())+" parts!";
		ret=false;
	}
	return ret;
}

bool ServicesBackend::DoCommand(Command cmd, std::promise<Command> result){
	if(verbosity>10) std::cout<<"ServicesBackend::DoCommand received command"<<std::endl;
	// submit a command, wait for the response and return it
	
	// capture a unique id for this message
	uint32_t thismsgid = ++msg_id;
	cmd.msg_id = thismsgid;
	
	// zmq sockets aren't thread-safe, so we have one central sender.
	// we submit our command and keep a ticket to retrieve the return status on completion.
	// similarly, the next response received may not be for us, so a central dealer receives
	// all responses and deals them out to the appropriate recipient. Again we register
	// ourselves as a recipient for the response, and wait for it to be fulfilled.
	// due to the asynchronous nature of these calls, we must register ourselves
	// as a recipient for a response before we even send the request, to ensure that
	// we can be identified as a the recipient as soon as we submit our command.
	// It's a little odd, but that's how it is.
	if(verbosity>10) std::cout<<"ServicesBackend::DoCommand pre-emptively submitting ticket for response"<<std::endl;
	std::promise<Command> response_ticket;
	std::future<Command> response_reciept = response_ticket.get_future();
	send_queue_mutex.lock();
	waiting_recipients.emplace(thismsgid, std::move(response_ticket));
	send_queue_mutex.unlock();
	
	// submit a request to send our command.
	std::promise<int> send_ticket;
	std::future<int> send_receipt = send_ticket.get_future();
	send_queue_mutex.lock();
	if(verbosity>10) std::cout<<"ServicesBackend::DoCommand putting command into waiting-to-send list"<<std::endl;
	waiting_senders.emplace(cmd, std::move(send_ticket));
	send_queue_mutex.unlock();
	
	// wait for our number to come up. loooong timeout, but don't hang forever.
	if(verbosity>10) std::cout<<"ServicesBackend::DoCommand waiting for send confirmation"<<std::endl;
	if(send_receipt.wait_for(std::chrono::seconds(30))==std::future_status::timeout){
		if(verbosity>10) std::cerr<<"ServicesBackend::DoCommand timeout"<<std::endl;
		// sending timed out
		if(cmd.type=='w') ++write_commands_failed;
		else if(cmd.type=='r') ++read_commands_failed;
		Log("Timed out sending command "+std::to_string(thismsgid),v_warning,verbosity);
		cmd.success = false;
		cmd.err = "Timed out sending command";
		result.set_value(cmd);
		
		// since we are giving up waiting for the response, remove ourselves from
		// the list of waiting recipients
		if(verbosity>10) std::cout<<"ServicesBackend::DoCommand de-registering for response on id "<<thismsgid<<std::endl;
		send_queue_mutex.lock();
		waiting_recipients.erase(thismsgid);
		send_queue_mutex.unlock();
		
		return true;
	} // else got a return value
	
	// so we got response about our send request, but did it go through?
	// check for errors sending
	if(verbosity>10) std::cout<<"ServicesBackend::DoCommand got send confirmation"<<std::endl;
	int ret = send_receipt.get();
	std::string errmsg;
	if(ret==-3) errmsg="Error polling out socket in PollAndSend! Is socket closed?";
	if(ret==-2) errmsg="No listener on out socket in PollAndSend!";
	if(ret==-1) errmsg="Error sending in PollAndSend!";
	if(ret!=0){
		if(verbosity>10) std::cout<<"ServicesBackend::DoCommand got response "<<ret
		                          <<" from PollAndSend: sending failed"<<std::endl;
		if(cmd.type=='w') ++write_commands_failed;
		else if(cmd.type=='r') ++read_commands_failed;
		Log(errmsg,v_debug,verbosity);
		cmd.success = false;
		cmd.err = errmsg;
		result.set_value(cmd);
		
		// since the send failed we don't expect a response, so remove ourselves
		// from the list of recipients awaiting response
		send_queue_mutex.lock();
		if(verbosity>10) std::cout<<"ServicesBackend::DoCommand de-registering for response on command "<<thismsgid<<std::endl;
		waiting_recipients.erase(thismsgid);
		send_queue_mutex.unlock();
		
		return false;
	} else {
		if(verbosity>10) std::cout<<"ServicesBackend::DoCommand message sent successfully"<<std::endl;
	}
	
	// if we succeeded in sending the message, we now need to wait for a repsonse.
	if(verbosity>10) std::cout<<"ServicesBackend::DoCommand waiting for response"<<std::endl;
	if(response_reciept.wait_for(std::chrono::seconds(30))==std::future_status::timeout){
		if(verbosity>10) std::cout<<"ServicesBackend::DoCommand response timeout"<<std::endl;
		// timed out
		if(cmd.type=='w') ++write_commands_failed;
		else if(cmd.type=='r') ++read_commands_failed;
		Log("Timed out waiting for response for command "+std::to_string(thismsgid),v_warning,verbosity);
		cmd.success = false;
		cmd.err = "Timed out waiting for response";
		result.set_value(cmd);
		return false;
	} else {
		// got a response!
		if(verbosity>10) std::cout<<"ServicesBackend::DoCommand got a response for command "<<cmd.msg_id
		                          <<", passing back to caller"<<std::endl;
		try{
			result.set_value(response_reciept.get());
		} catch(std::exception& e){
			Log("ServicesBackend response for command "+std::to_string(cmd.msg_id)+" was exception "+e.what(),v_error,verbosity);
			cmd.err=e.what();
			cmd.success=false;
			result.set_value(cmd);
			return false;
		}
		return true;
	}
	
	return true;  // dummy
	
}

bool ServicesBackend::GetNextResponse(){
	// get any new messages from middleman, and notify the client of the outcome
	
	std::vector<zmq::message_t> response;
	dlr_socket_mutex.lock();
	int ret = PollAndReceive(clt_dlr_socket, in_polls.at(0), inpoll_timeout, response);
	dlr_socket_mutex.unlock();
	//std::cout<<"ServicesBackend: GNR returned "<<ret<<std::endl;
	
	// check return status
	if(ret==-2) return true;      // no messages waiting to be received
	
	if(verbosity>10) std::cout<<"ServicesBackend::GetNextResponse had response in socket"<<std::endl;
	if(ret==-3){
		Log("PollAndReceive Error polling in socket! Is socket closed?",v_error,verbosity);
		return false;
	}
	
	if(response.size()==0){
		Log("PollAndReceive received empty response!",v_error,verbosity);
		return false;
	}
	
	// received message may be an acknowledgement of a write, or the result of a read.
	// messages are 2+ zmq parts as follows:
	// 1. the message ID, used by the client to match to the message it sent
	// 2. the response code, to signal errors
	// 3.... the command results, if any. May consist of several parts.
	Command cmd;
	if(ret==-1 || response.size()<2){
		// return of -1 suggests the last zmq message had the 'more' flag set
		// suggesting there should have been more parts, but they never came.
		cmd.success = false;
		cmd.err="Received incomplete zmq response";
		Log(cmd.err,v_warning,verbosity);
		if(ret==-1) Log("Last message had zmq more flag set",v_warning,verbosity);
		if(response.size()<2) Log("Only received "+std::to_string(response.size())+" parts",v_warning,verbosity);
		// continue to parse as much as we can - the first part identifies the command,
		// so we can at least inform the client of the failure
	}
	// else if ret==0 && response.size() >= 2: success
	
	// if we got this far we had at least one response part; the message id
	uint32_t message_id_rcvd = *reinterpret_cast<uint32_t*>(response.at(0).data());
	
	// if we also had a status part, get that
	if(response.size()>1){
		cmd.success = *reinterpret_cast<uint32_t*>(response.at(1).data());  // 1=success, 0=fail
	}
	// if we also had further parts, fetch those
	// if the command failed the response contains an error message (which will only ever be one part)
	for(unsigned int i=2; i<response.size(); ++i){
		//cmd.response.push_back(std::string(reinterpret_cast<const char*>(response.at(i).data())));
		std::string resp(response.at(i).size(),'\0');
		memcpy((void*)resp.data(), response.at(i).data(), response.at(i).size());
		resp = resp.substr(0,resp.find('\0'));
		if(cmd.success) cmd.response.push_back(resp);
		else cmd.err = resp;
	}
	
	
	if(verbosity>3){
		std::stringstream logmsg;
		logmsg << "ServicesBackend::GetNextResponse received response to command "<<message_id_rcvd
		       <<"; status "<<cmd.success<<", response '";
		for(auto& apart : cmd.response){
			logmsg<<"["<<apart<<"]";
		}
		logmsg<<"'"<<std::endl;
		Log(logmsg.str(),v_debug,verbosity);
	}
	
	// get the ticket associated with this message id
	if(waiting_recipients.count(message_id_rcvd)){
		std::promise<Command>* ticket = &waiting_recipients.at(message_id_rcvd);
		ticket->set_value(cmd);
		// remove it from the map of waiting promises
		send_queue_mutex.lock();
		waiting_recipients.erase(message_id_rcvd);
		send_queue_mutex.unlock();
	} else {
		// unknown message id?
		Log("Unknown message id "+std::to_string(message_id_rcvd)+" with no client",v_error,verbosity);
		return false;
	}
	
	return true;
}

bool ServicesBackend::SendNextCommand(){
	// send the next message in the waiting command queue
	
	if(waiting_senders.empty()){
		// nothing to send
		return true;
	}
	if(verbosity>10) std::cout<<"ServicesBackend::SendNextCommand got outgoing command to send"<<std::endl;
	
	// get the next command to send
	send_queue_mutex.lock();
	std::pair<Command, std::promise<int>> next_cmd = std::move(waiting_senders.front());
	waiting_senders.pop();
	send_queue_mutex.unlock();
	Command& cmd = next_cmd.first;
	if(verbosity>10) std::cout<<"ServicesBackend::SendNextCommand sending command "<<cmd.msg_id<<std::endl;
	if(verbosity>3){
		std::stringstream logmsg;
		logmsg<<"Sending command "<<cmd.msg_id<<", \""<<cmd.command<<"\""<<std::endl;
		//Log(logmsg.str(),v_debug,verbosity);
	}
	
	// write commands go to the pub socket, read commands to the dealer
	zmq::socket_t* thesocket = (cmd.type=='w') ? clt_pub_socket : clt_dlr_socket;
	
	// send out the command
	// commands should be formatted as 4 parts:
	// 0. pub topic (used by ZMQ_SUBSCRIBE for pub, but send it always)
	// 1. client ID
	// 2. message ID
	// 4. command
	// XXX note! read commands go out via a Dealer socket, which automatically prepends
	// the ZMQ identity of the sender. Writes go out via a Pub socket that does not!
	int ret=-99;
	dlr_socket_mutex.lock();
	if(verbosity>10) std::cout<<"ServicesBackend::SendNextCommand calling PollAndSend"
	                          <<", message type: "<<cmd.type<<", topic '"<<cmd.topic<<"'"<<std::endl;
	if(cmd.type=='w'){
		ret = PollAndSend(thesocket, out_polls.at(1), outpoll_timeout, cmd.topic, clt_ID, cmd.msg_id, cmd.command);
	} else {
		// clt_ID already added by dealer socket
		ret = PollAndSend(thesocket, out_polls.at(1), outpoll_timeout, cmd.topic, cmd.msg_id, cmd.command);
	}
	if(verbosity>10) std::cout<<"ServicesBackend::SendNextCommand send returned "<<ret<<", passing to recipient"<<std::endl;
	dlr_socket_mutex.unlock();
	//std::cout<<"ServicesBackend SNC P&S returned "<<ret<<std::endl;
	
	// notify the client that the message has been sent
	next_cmd.second.set_value(ret);
	
	return true;
	
}

bool ServicesBackend::Finalise(){
	// terminate our background thread
	Log("ServicesBackend sending background thread term signal",v_debug,verbosity);
	terminator.set_value();
	// wait for it to finish up and return
	Log("ServicesBackend waiting for background thread to rejoin",v_debug,verbosity);
	background_thread.join();
	
	Log("ServicesBackend Removing services",v_debug,verbosity);
	if(utilities) utilities->RemoveService("slowcontrol_write");
	if(utilities) utilities->RemoveService("slowcontrol_read");
	
	Log("ServicesBackend Closing multicast socket",v_debug,verbosity);
	close(multicast_socket);
	
	Log("ServicesBackend Deleting Utilities class",v_debug,verbosity);
	if(utilities){
	  delete utilities;
	  utilities=nullptr;
	}
	
	// clear old connections
	clt_pub_connections.clear();
	clt_dlr_connections.clear();
	
	Log("ServicesBackend deleting sockets",v_debug,verbosity);
	delete clt_pub_socket; clt_pub_socket=nullptr;
	delete clt_dlr_socket; clt_dlr_socket=nullptr;
	
	// clear old associated polls
	in_polls.clear();
	out_polls.clear();
	
	// clear old commands and responses
	waiting_senders = std::queue<std::pair<Command, std::promise<int>>>{};
	waiting_recipients.clear();
	
	// can't use 'Log' since we may have deleted the Logging class
	if(verbosity>3) std::cout<<"ServicesBackend finalise done"<<std::endl;
	
	return true;
}

// =====================================================================
// ZMQ helper functions; TODO move these to external class? since they're shared by middleman.

bool ServicesBackend::Send(zmq::socket_t* sock, bool more, zmq::message_t& message){
	if(verbosity>10) std::cout<<__PRETTY_FUNCTION__<<" called"<<std::endl;
	bool send_ok;
	if(more){
		send_ok = sock->send(message, ZMQ_SNDMORE);
		if(verbosity>10) std::cout<<"zmq sent next part: "<<send_ok<<std::endl;
	} else {
		send_ok = sock->send(message);
		if(verbosity>10) std::cout<<"zmq send final part: "<<send_ok<<std::endl;
	}
	
	if(verbosity>10) std::cout<<"returning "<<send_ok<<std::endl;
	return send_ok;
}

bool ServicesBackend::Send(zmq::socket_t* sock, bool more, std::string messagedata){
	if(verbosity>10) std::cout<<__PRETTY_FUNCTION__<<" called"<<std::endl;
	// form the zmq::message_t
	zmq::message_t message(messagedata.size());
	//snprintf((char*)message.data(), messagedata.size()+1, "%s", messagedata.c_str());
	memcpy(message.data(), messagedata.data(), message.size());
	
	// send it with given SNDMORE flag
	bool send_ok;
	if(more){
		send_ok = sock->send(message, ZMQ_SNDMORE);
		if(verbosity>10) std::cout<<"zmq sent next part: "<<send_ok<<std::endl;
	} else {
		send_ok = sock->send(message);
		if(verbosity>10) std::cout<<"zmq send final part: "<<send_ok<<std::endl;
	}
	
	if(verbosity>10) std::cout<<"returning "<<send_ok<<std::endl;
	return send_ok;
}

bool ServicesBackend::Send(zmq::socket_t* sock, bool more, std::vector<std::string> messages){
	if(verbosity>10) std::cout<<__PRETTY_FUNCTION__<<" called"<<std::endl;
	
	// loop over all but the last part in the input vector,
	// and send with the SNDMORE flag
	for(unsigned int i=0; i<(messages.size()-1); ++i){
		if(verbosity>10) std::cout<<"sending part "<<i<<"/"<<messages.size()<<std::endl;
		
		// form zmq::message_t
		zmq::message_t message(messages.at(i).size());
		memcpy(message.data(), messages.at(i).data(), messages.at(i).size());
		//snprintf((char*)message.data(), messages.at(i).size()+1, "%s", messages.at(i).c_str());
		
		// send this part
		bool send_ok = sock->send(message, ZMQ_SNDMORE);
		if(verbosity>10) std::cout<<"zmq sent next part: "<<send_ok<<std::endl;
		
		if(verbosity>10) std::cout<<"returned "<<send_ok<<std::endl;
		// break on error
		if(not send_ok) return false;
	}
	
	if(verbosity>10) std::cout<<"sending part "<<(messages.size()-1)<<"/"<<messages.size()<<std::endl;
	// form the zmq::message_t for the last part
	zmq::message_t message(messages.back().size());
	memcpy(message.data(), messages.back().data(), messages.back().size());
	//snprintf((char*)message.data(), messages.back().size()+1, "%s", messages.back().c_str());
	
	// send it with, or without SNDMORE flag as requested
	bool send_ok;
	if(more){
		send_ok = sock->send(message, ZMQ_SNDMORE);
		if(verbosity>10) std::cout<<"zmq sent next part: "<<send_ok<<std::endl;
	} else {
		send_ok = sock->send(message);
		if(verbosity>10) std::cout<<"zmq send final part: "<<send_ok<<std::endl;
	}
	if(verbosity>10) std::cout<<"returned "<<send_ok<<std::endl;
	
	return send_ok;
}

int ServicesBackend::PollAndReceive(zmq::socket_t* sock, zmq::pollitem_t poll, int timeout, std::vector<zmq::message_t>& outputs){
	
	// poll the input socket for messages
	try {
		get_ok = zmq::poll(&poll, 1, timeout);
	} catch (zmq::error_t& err){
		std::cerr<<"ServicesBackend::PollAndReceive poller caught "<<err.what()<<std::endl;
		get_ok = -1;
	}
	if(get_ok<0){
		// error polling - is the socket closed?
		return -3;
	}
	
	// check for messages waiting to be read
	if(poll.revents & ZMQ_POLLIN){
		
		// recieve all parts
		get_ok = Receive(sock, outputs);
		if(not get_ok) return -1;
		
	} else {
		// no waiting messages
		return -2;
	}
	// else received ok
	return 0;
}

bool ServicesBackend::Receive(zmq::socket_t* sock, std::vector<zmq::message_t>& outputs){
	
	outputs.clear();
	//int part=0;
	
	// recieve parts into tmp variable
	zmq::message_t tmp;
	while(sock->recv(&tmp)){
		
		// transfer the received message to the output vector
		outputs.resize(outputs.size()+1);
		outputs.back().move(&tmp);
		
		// receive next part if there is more to come
		if(!outputs.back().more()) break;
		
	}
	
	// if we broke the loop but last successfully received message had a more flag,
	// we must have broken due to a failed receive
	if(outputs.back().more()){
		// sock->recv failed
		return false;
	}
	
	// otherwise no more parts. done.
	return true;
}

