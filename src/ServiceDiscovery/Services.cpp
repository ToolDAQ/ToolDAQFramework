#include <Services.h>

using namespace ToolFramework;

Services::Services(){
  m_context=0;
  m_dbname="";
  m_name="";

}
 
Services::~Services(){
  
  m_backend_client.Finalise();
  sc_vars->Stop();
  m_context=nullptr;
  
}

bool Services::Init(Store &m_variables, zmq::context_t* context_in, SlowControlCollection* sc_vars_in, bool new_service){

  m_context = context_in;
  sc_vars = sc_vars_in;
  sc_vars->InitThreadedReceiver(m_context, 60000, 100, new_service);
  m_backend_client.SetUp(m_context);
  
  if(!m_variables.Get("service_name",m_name)) m_name="test_service";
  if(!m_variables.Get("db_name",m_dbname)) m_dbname="daq";
  
  
    
  if(!m_backend_client.Initialise(m_variables)){
    std::cerr<<"error initialising slowcontrol client"<<std::endl;
    return false;
  }
   
  // TODO FIXME with better mechanism
  // after Initilising the slow control client needs ~15 seconds for the middleman to connect
  std::this_thread::sleep_for(std::chrono::seconds(15));
  // hopefully the middleman has found us by now
  
  return true;
}

// ===========================================================================
// Write Functions
// ---------------

bool Services::SendAlarm(const std::string& message, unsigned int level, const std::string& device, unsigned int timestamp, const unsigned int timeout){
  
  const std::string& name = (device=="") ? m_name : device;
  
  // we only handle 2 levels of alarm: critical (level 0) and normal (level!=0).
  if(level>0) level=1;
  
  std::string cmd_string = "{\"time\":"+std::to_string(timestamp)
                         + ",\"device\":\""+name+"\""
                         + ",\"level\":"+std::to_string(level)
                         + ",\"message\":\"" + message + "\"}";
  
  std::string err="";
  
  // send the alarm on the pub socket
  bool ok = m_backend_client.SendCommand("W_ALARM", cmd_string, (std::vector<std::string>*)nullptr, &timeout,  &err);
  if(!ok){
    std::cerr<<"SendAlarm error: "<<err<<std::endl;
  }
  
  // also record it to the logging socket
  cmd_string = std::string{"{ \"topic\":\"logging\""}
             + ",\"time\":"+std::to_string(timestamp)
             + ",\"device\":\""+name+"\""
             + ",\"severity\":0"
             + ",\"message\":\"" + message + "\"}";
  
  ok = m_backend_client.SendMulticast(cmd_string, &err);
  
  if(!ok){
    std::cerr<<"SendAlarm (log) error: "<<err<<std::endl;
  }
  
  return ok;
  
}

bool Services::SendCalibrationData(const std::string& json_data, const std::string& description, const std::string& device, unsigned int timestamp, int* version, const unsigned int timeout){
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = "{ \"time\":"+std::to_string(timestamp)
                         + ", \"device\":\""+ name +"\""
                         + ", \"description\":\""+ description+"\""
                         + ", \"data\":\""+ json_data +"\" }";
  
  std::string err="";
  std::string response="";
  
  if(!m_backend_client.SendCommand("W_CALIBRATION", cmd_string, &response, &timeout, &err)){
    std::cerr<<"SendCalibrationData error: "<<err<<std::endl;
    return false;
  }
  
  // response is json with the version number of the created config entry
  // e.g. '{"version":"3"}'. check this is what we got, as validation.
  if(response.length()>14){
    response.replace(0,12,"");
    response.replace(response.end()-2, response.end(),"");
    try {
      if(version) *version = std::stoi(response);
    } catch (...){
      std::cerr<<"SendConfig error: invalid response '"<<response<<"'"<<std::endl;
      return false;
    }
  } else {
    std::cerr<<"SendConfig error: invalid response: '"<<response<<"'"<<std::endl;
    return false;
  }
  
  return true;
  
}

bool Services::SendConfig(const std::string& json_data, const std::string& author, const std::string& description, const std::string& device, unsigned int timestamp, int* version, const unsigned int timeout){
  
  if(version) *version=-1;
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = "{ \"time\":"+std::to_string(timestamp)
                         + ", \"device\":\""+ name+"\""
                         + ", \"author\":\""+ author+"\""
                         + ", \"description\":\""+ description+"\""
                         + ", \"data\":\""+ json_data +"\" }";
  
  std::string response="";
  std::string err="";
  
  if(!m_backend_client.SendCommand("W_CONFIG", cmd_string, &response, &timeout, &err)){
    std::cerr<<"SendConfig error: "<<err<<std::endl;
    return false;
  }
  
  // response is json with the version number of the created config entry
  // e.g. '{"version":"3"}'. check this is what we got, as validation.
  if(response.length()>14){
    response.replace(0,12,"");
    response.replace(response.end()-2, response.end(),"");
    try {
      if(version) *version = std::stoi(response);
    } catch (...){
      std::cerr<<"SendConfig error: invalid response '"<<response<<"'"<<std::endl;
      return false;
    }
  } else {
    std::cerr<<"SendConfig error: invalid response: '"<<response<<"'"<<std::endl;
    return false;
  }
  
  return true;
  
}

// ===========================================================================
// Read Functions
// --------------

bool Services::GetCalibrationData(std::string& json_data, int version, const std::string& device, const unsigned int timeout){
  
  json_data = "";
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = "{ \"device\":\""+ name +"\""
                         + ", \"version\":\""+std::to_string(version)+"\" }";
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_CALIBRATION", cmd_string, &json_data, &timeout, &err)){
    std::cerr<<"GetCalibrationData error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  return true;
  
}

bool Services::GetConfig(std::string& json_data, int version, const std::string& device, const unsigned int timeout){

  json_data="";
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = "{ \"device\":\""+ name +"\""
                         + ", \"version\":"+std::to_string(version) + "}";
  
  std::string err="";
  
  if(m_backend_client.SendCommand("R_CONFIG", cmd_string, &json_data, &timeout, &err)){
    std::cerr<<"GetConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  // response format '{"data":"<contents>"}' - strip out contents
  if(json_data.length()>11){
    json_data.replace(0,9,"");
    json_data.replace(json_data.end()-2, json_data.end(),"");
  } else {
    std::cerr<<"GetConfig error: invalid response: '"<<json_data<<"'"<<std::endl;
    return false;
  }
  
  return true;
  
}

bool Services::GetROOTplot(const std::string& plot_name, int& version, std::string& draw_options, std::string& json_data, std::string* timestamp, const unsigned int timeout){
  
  std::string cmd_string = "{ \"plot_name\":\""+ plot_name + "\""
                         + ", \"version\":" + std::to_string(version)+ "}";
  
  std::string err="";
  std::string response="";
  if(!m_backend_client.SendCommand("R_ROOTPLOT", cmd_string, &response, &timeout, &err)){
    std::cerr<<"GetROOTplot error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(response.empty()){
    std::cout<<"GetROOTplot error: empty response, "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  // response format '{"draw_options":"<options>", "timestamp":<value>, "version": <value>, "data":"<contents>"}'
  size_t pos1=0, pos2=0;
  std::string key;
  std::map<std::string,std::string> vals;
  while(true){
    pos1=response.find('"',pos2);
    if(pos1==std::string::npos) break;
    pos2=response.find('"',++pos1);
    if(pos2==std::string::npos) break;
    std::string str = response.substr(pos1,(pos2-pos1));
    ++pos2;
    if(key.empty()){ key = str; }
    else if(key=="data"){ break; }
    else { vals[key] = str; key=""; }
  }
  vals[key] = response.substr(pos1,response.find_last_of('"')-pos1);
  
  try{
    draw_options = vals["draw_options"];
    if(timestamp) *timestamp = vals["timestamp"];
    version = std::stoi(vals["version"]);
    json_data = vals["data"];
  } catch(...){
    std::cerr<<"GetROOTplot error: failed to parse response '"<<response<<"'"<<std::endl;
    json_data = "Parse error";
    return false;
  }
  
  /*
  std::cout<<"timestamp: "<<timestamp<<"\n"
           <<"draw opts: "<<draw_options<<"\n"
           <<"json data: '"<<json_data<<"'"<<std::endl;
  */
  
  return true;
  
}

bool Services::SQLQuery(const std::string& database, const std::string& query, std::vector<std::string>* responses, const unsigned int timeout){
  
  if(responses) responses->clear();
  
  const std::string& db = (database=="") ? m_dbname : database;
  
  const std::string command = "{ \"database\":\""+db+"\""
                            + ", \"query\":\""+ query+"\" }";
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_QUERY", command, responses, &timeout, &err)){
    std::cerr<<"SQLQuery error: "<<err<<std::endl;
    if(responses){
      responses->resize(1);
      responses->front() = err;
    }
    return false;
  }
  
  return true;
  
}

bool Services::SQLQuery(const std::string& query, const std::string& database, std::string* response, const unsigned int timeout){
  
  if(response) *response="";
  
  const std::string& db = (database=="") ? m_dbname : database;
  
  std::vector<std::string> responses;
  
  bool ok = !SQLQuery(db, query, &responses, timeout);
  
  if(response!=nullptr && responses.size()!=0){
    *response = responses.front();
    if(responses.size()>1){
      std::cerr<<"Warning: SQLQuery returned multiple rows, only first returned"<<std::endl;
    }
  }
  
  return ok;
}

// ===========================================================================
// Multicast Senders
// -----------------

bool Services::SendLog(const std::string& message, unsigned int severity, const std::string& device, unsigned int timestamp){
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = std::string{"{ \"topic\":\"logging\""}
                         + ",\"time\":"+std::to_string(timestamp)
                         + ",\"device\":\""+ name +"\""
                         + ",\"severity\":"+std::to_string(severity)
                         + ",\"message\":\"" +  message + "\"}";
  
  if(cmd_string.length()>655355){
    std::cerr<<"Logging message is too long! Maximum length may be 655355 bytes"<<std::endl;
    return false;
  }
  
  std::string err="";
  
  if(!m_backend_client.SendMulticast(cmd_string, &err)){
    std::cerr<<"SendLog error: "<<err<<std::endl;
    return false;
  }
  
  return true;
  
}


bool Services::SendMonitoringData(const std::string& json_data, const std::string& device, unsigned int timestamp){
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = std::string{"{ \"topic\":\"monitoring\""}
                         + ", \"time\":"+std::to_string(timestamp)
                         + ", \"device\":\""+ name +"\""
                         + ", \"data\":\""+ json_data +"\" }";
  
  if(cmd_string.length()>655355){
    std::cerr<<"Monitoring message is too long! Maximum length may be 655355 bytes"<<std::endl;
    return false;
  }
  
  std::string err="";
  
  if(!m_backend_client.SendMulticast(cmd_string, &err)){
    std::cerr<<"SendMonitoringData error: "<<err<<std::endl;
    return false;
  }
  
  return true;
  
}

// wrapper to send a root plot either to a temporary table or a persistent one
bool Services::SendROOTplot(const std::string& plot_name, const std::string& draw_options, const std::string& json_data, bool persistent, int* version, const unsigned int timestamp, const unsigned int timeout){
  if(!persistent) return SendTemporaryROOTplot(plot_name, draw_options, json_data, version, timestamp);
  return SendPersistentROOTplot(plot_name, draw_options, json_data, version, timestamp, timeout);
}

// send to persistent table over TCP
bool Services::SendPersistentROOTplot(const std::string& plot_name, const std::string& draw_options, const std::string& json_data, int* version, const unsigned int timestamp, const unsigned int timeout){
  
  std::string cmd_string = "{ \"time\":"+std::to_string(timestamp)
                         + ", \"plot_name\":\""+ plot_name +"\""
                         + ", \"draw_options\":\""+ draw_options +"\""
                         + ", \"data\":\""+ json_data+"\" }";
  
  std::string err="";
  std::string response="";
  
  if(!m_backend_client.SendCommand("W_ROOTPLOT", cmd_string, &response, &timeout, &err)){
    std::cerr<<"SendROOTplot error: "<<err<<std::endl;
    return false;
  }
  
  // response is json with the version number of the created plot entry
  // e.g. '{"version":"3"}'. check this is what we got, as validation.
  if(response.length()>14){
    response.replace(0,12,"");
    response.replace(response.end()-2, response.end(),"");
    try {
      if(version) *version = std::stoi(response);
    } catch (...){
      std::cerr<<"SendROOTplot error: invalid response '"<<response<<"'"<<std::endl;
      return false;
    }
  } else {
    std::cerr<<"SendROOTplot error: invalid response: '"<<response<<"'"<<std::endl;
    return false;
  }
  
  return true;
  
}

// send to temporary table over multicast
bool Services::SendTemporaryROOTplot(const std::string& plot_name, const std::string& draw_options, const std::string& json_data, int* version, const unsigned int timestamp){
  
  std::string cmd_string = std::string{"{ \"topic\":\"rootplot\""}
                         + ", \"time\":"+std::to_string(timestamp)
                         + ", \"plot_name\":\""+ plot_name +"\""
                         + ", \"draw_options\":\""+ draw_options +"\""
                         + ", \"data\":\""+ json_data+"\" }";
  
  if(cmd_string.length()>655355){
    std::cerr<<"ROOT plot json is too long! Maximum length may be 655355 bytes"<<std::endl;
    return false;
  }
  
  std::string err="";
  
  if(!m_backend_client.SendMulticast(cmd_string, &err)){
    std::cerr<<"SendROOTplot error: "<<err<<std::endl;
    return false;
  }
  
  return true;
  
}

static std::vector<float> decodeArray(const std::string& string) {
  if (string.length() < 2) return std::vector<float>(); // "{}"
  size_t length = 1;
  for (size_t i = 0; i < string.size(); ++i) if (string[i] == ',') ++length;

  std::vector<float> result(length);

  std::stringstream ss;
  ss << string;
  size_t i = 0;
  char c;
  ss >> c; // '{'
  while (ss && c != '}' && i < length) ss >> result[i++] >> c;
  return result;
}

static std::string encodeArray(const std::vector<float>& array) {
  std::stringstream ss;
  ss << "'{";
  bool first = true;
  for (float x : array) {
    if (!first) ss << ',';
    first = false;
    ss << x;
  };
  ss << "}'";
  return ss.str();
}

bool Services::GetPlot(const std::string& name, Plot& plot, unsigned timeout) {
  std::string cmd = "{\"name\":\"" + name + "\"}";
  std::string err = "";
  std::string response;
  if (!m_backend_client.SendCommand("R_PLOT", cmd, &response, &timeout, &err)) {
    std::cerr << "GetPlot error: " << err << std::endl;
    return false;
  };

  plot.name = name;

  Store data;
  data.JsonParser(response);

#define get(slot, var) if (!data.Get(slot, var)) return false;
  std::string array;
  get("x", array);
  plot.x = decodeArray(array);
  get("y", array);
  plot.y = decodeArray(array);
  get("title",  plot.title);
  get("xlabel", plot.xlabel);
  get("ylabel", plot.ylabel);
  get("info",   plot.info);
#undef get

  return true;
}

bool Services::SendPlot(Plot& plot, unsigned timeout) {
  Store data;
  data.Set("name",   plot.name);
  data.Set("x",      encodeArray(plot.x));
  data.Set("y",      encodeArray(plot.y));
  data.Set("title",  plot.title);
  data.Set("xlabel", plot.xlabel);
  data.Set("ylabel", plot.ylabel);

  std::string string;
  plot.info >> string;
  data.Set("info", string);

  data >> string;
  std::string err;
  if (!m_backend_client.SendCommand(
        "W_PLOT", string, static_cast<std::string*>(nullptr), &timeout, &err
       ))
  {
    std::cerr << "SendPlot error: " << err << std::endl;
    return false;
  };

  return true;
}

// ===========================================================================
// Other functions
// ---------------

SlowControlCollection* Services::GetSlowControlCollection(){
  
  return sc_vars;

}

SlowControlElement* Services::GetSlowControlVariable(std::string key){
  
  return (*sc_vars)[key];
  
}

bool Services::AddSlowControlVariable(std::string name, SlowControlElementType type, std::function<std::string(const char*)> change_function, std::function<std::string(const char*)> read_function){
  
  return sc_vars->Add(name, type, change_function, read_function);
  
}

bool Services::RemoveSlowControlVariable(std::string name){
  
  return sc_vars->Remove(name);
  
}

void Services::ClearSlowControlVariables(){

  sc_vars->Clear();

}

bool Services::AlertSubscribe(std::string alert, std::function<void(const char*, const char*)> function){
  
  return sc_vars->AlertSubscribe(alert, function);
  
}
bool Services::AlertSend(std::string alert, std::string payload){
  
  return sc_vars->AlertSend(alert, payload);
  
}

std::string Services::PrintSlowControlVariables(){
  
  return sc_vars->Print();
  
}

std::string Services::GetDeviceName(){
  
  return m_name;
  
}
