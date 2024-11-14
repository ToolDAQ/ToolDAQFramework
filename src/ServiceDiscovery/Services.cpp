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
    std::clog<<"error initialising slowcontrol client"<<std::endl;
    return false;
  }
   
  // wait for the middleman to connect. ServiceDiscovery broadcasts are sent out intermittently,
  // so we need to wait for the middleman to receive one & connect before we can communicate with it.
  int pub_period=5;
  m_variables.Get("service_publish_sec",pub_period);
  Ready(pub_period*1000);
  
  return true;
}

bool Services::Ready(const unsigned int timeout){
  return m_backend_client.Ready(timeout);
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
    std::clog<<"SendAlarm error: "<<err<<std::endl;
  }
  
  // also record it to the logging socket
  cmd_string = std::string{"{ \"topic\":\"logging\""}
             + ",\"time\":"+std::to_string(timestamp)
             + ",\"device\":\""+name+"\""
             + ",\"severity\":0"
             + ",\"message\":\"" + message + "\"}";
  
  ok = ok && m_backend_client.SendMulticast(cmd_string, &err);
  
  if(!ok){
    std::clog<<"SendAlarm (log) error: "<<err<<std::endl;
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
    std::clog<<"SendCalibrationData error: "<<err<<std::endl;
    return false;
  }
  
  // response is json with the version number of the created config entry
  // e.g. '{"version":3}'. check this is what we got, as validation.
  Store tmp;
  tmp.JsonParser(response);
  int tmp_version;
  bool ok = tmp.Get("version",tmp_version);
  if(version){
    *version = tmp_version;
  }
  if(!ok){
    std::clog<<"SendCalibrationData error: invalid response: '"<<response<<"'"<<std::endl;
    return false;
  }
  
  return true;
  
}

bool Services::SendDeviceConfig(const std::string& json_data, const std::string& author, const std::string& description, const std::string& device, unsigned int timestamp, int* version, const unsigned int timeout){
  
  if(version) *version=-1;
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = "{ \"time\":"+std::to_string(timestamp)
                         + ", \"device\":\""+ name+"\""
                         + ", \"author\":\""+ author+"\""
                         + ", \"description\":\""+ description+"\""
                         + ", \"data\":\""+ json_data +"\" }";
  
  std::string response="";
  std::string err="";
  
  if(!m_backend_client.SendCommand("W_DEVCONFIG", cmd_string, &response, &timeout, &err)){
    std::clog<<"SendDeviceConfig error: "<<err<<std::endl;
    return false;
  }
  
  // response is json with the version number of the created config entry
  // e.g. '{"version":3}'. check this is what we got, as validation.
  Store tmp;
  tmp.JsonParser(response);
  int tmp_version;
  bool ok = tmp.Get("version",tmp_version);
  if(version){
    *version = tmp_version;
  }
  if(!ok){
    std::clog<<"SendDeviceConfig error: invalid response: '"<<response<<"'"<<std::endl;
    return false;
  }
  
  return true;
  
}

bool Services::SendRunConfig(const std::string& json_data, const std::string& name, const std::string& author, const std::string& description, unsigned int timestamp, int* version, const unsigned int timeout){
  
  if(version) *version=-1;
  
  std::string cmd_string = "{ \"time\":"+std::to_string(timestamp)
                         + ", \"name\":\""+ name+"\""
                         + ", \"author\":\""+ author+"\""
                         + ", \"description\":\""+ description+"\""
                         + ", \"data\":\""+ json_data +"\" }";
  
  std::string response="";
  std::string err="";
  
  if(!m_backend_client.SendCommand("W_RUNCONFIG", cmd_string, &response, &timeout, &err)){
    std::clog<<"SendRunConfig error: "<<err<<std::endl;
    return false;
  }
  
  // response is json with the version number of the created config entry
  // e.g. '{"version":3}'. check this is what we got, as validation.
  Store tmp;
  tmp.JsonParser(response);
  int tmp_version;
  bool ok = tmp.Get("version",tmp_version);
  if(version){
    *version = tmp_version;
  }
  if(!ok){
    std::clog<<"SendRunConfig error: invalid response: '"<<response<<"'"<<std::endl;
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
    std::clog<<"GetCalibrationData error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  return true;
  
}

bool Services::GetDeviceConfig(std::string& json_data, const int version, const std::string& device, const unsigned int timeout){

  json_data="";
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = "{ \"device\":\""+ name +"\""
                         + ", \"version\":"+std::to_string(version) + "}";
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_DEVCONFIG", cmd_string, &json_data, &timeout, &err)){
    std::clog<<"GetDeviceConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  // response format '{"version":X, "data":"<contents>"}' - strip out contents
  if(json_data.length()==0){
    // if we got an empty response but the command succeeded,
    // the query worked but matched no records - run config not found
    err = "GetDeviceConfig error: config for device "+name+" version "+std::to_string(version)+" not found";
    std::clog<<err<<std::endl;
    json_data = err;
    return false;
  }
  Store tmp;
  tmp.JsonParser(json_data);
  int tmp_version;
  bool ok = tmp.Get("version",tmp_version);
  if(ok){
    //version = tmp_version;  // cannot pass back... without a more complex signature
    ok = tmp.Get("data", json_data);
  }
  if(!ok){
    std::clog<<"GetDeviceConfig error: invalid response: '"<<json_data<<"'"<<std::endl;
    return false;
  }
  
  return true;
  
}

// get a run configuration via configuration ID
bool Services::GetRunConfig(std::string& json_data, const int config_id, const unsigned int timeout){

  json_data="";
  
  std::string cmd_string = "{ \"config_id\":"+std::to_string(config_id) + "}";
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_RUNCONFIG", cmd_string, &json_data, &timeout, &err)){
    std::clog<<"GetRunConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  // response format '{"data":<contents>}' - strip out contents
  Store tmp;
  tmp.JsonParser(json_data);
  bool ok = tmp.Get("data", json_data);
  if(!ok){
    std::clog<<"GetRunConfig error: invalid response: '"<<json_data<<"'"<<std::endl;
    return false;
  }
  
  return true;
  
}

// get a run configuration by name and version (e.g. name: AmBe, version: 3)
bool Services::GetRunConfig(std::string& json_data, const std::string& name, const int version, const unsigned int timeout){

  json_data="";
  
  std::string cmd_string = "{ \"name\":\""+ name +"\""
                         + ", \"version\":"+std::to_string(version) + "}";
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_RUNCONFIG", cmd_string, &json_data, &timeout, &err)){
    std::clog<<"GetRunConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  // response format '{"version":X, "data":"<contents>"}' - strip out contents
  if(json_data.length()==0){
    // if we got an empty response but the command succeeded,
    // the query worked but matched no records - run config not found
    std::clog<<"GetRunConfig error: config "<<name<<" version "<<version<<" not found"<<std::endl;
    return false;
  }
  Store tmp;
  tmp.JsonParser(json_data);
  int tmp_version;
  bool ok = tmp.Get("version",tmp_version);
  if(ok){
    //version = tmp_version;  // cannot pass back
    ok = tmp.Get("data", json_data);
  }
  if(!ok){
    err="GetRunConfig error: invalid response: '"+json_data+"'";
    std::clog<<err<<std::endl;
    json_data=err;
    return false;
  }
  
  return true;
  
}

// quick aside for a couple of convenience wrappers.
// For a device to get its configuration from a *run* configuration ID, it needs to:
// 1. get the run configuration JSON from that ID (this represents a map of devices to device config IDs)
// 2. extract its device configuration ID from this map
// 3. get its device configuration from the database via this device configuration ID.
// to make things easy for end users, provide a wrapper that does this.
// technically we have two wrappers as there are two ways to specify a run configuration (by id or by name+version)

bool Services::GetRunDeviceConfig(std::string& json_data, const int runconfig_id, const std::string& device, int* version, unsigned int timeout){
  
  json_data="";
  
  const std::string& name = (device=="") ? m_name : device;
  
  // 1. get the run configuration
  std::string run_config="";
  bool get_ok = GetRunConfig(run_config, runconfig_id, timeout/2);
  
  if(!get_ok){
    // redundant as GetRunConfig prints the error - but is this still useful as calling context?
    // TODO we should be using exceptions, of course
    //std::clog<<"GetRunDeviceConfig error getting run config id "<<runconfig_id<<": '"<<run_config<<"'"<<std::endl;
    json_data=run_config;
    return false;
  }
  
printf("GetRunConfig from GetRunDev returned: '%s'\n",run_config.c_str());
  // 2. extract the device's configuration id
  Store tmp;
  tmp.JsonParser(run_config);
  int device_config_id;
  get_ok = tmp.Get(name, device_config_id);
printf("tmp store contains:\n");
tmp.Print();
  
  if(!get_ok){
    std::string err= "GetRunDeviceConfig error getting device config; device '"+name
                   +"' not found in run configuration '"+run_config+"'";
    std::clog<<err<<std::endl;
    json_data = err;
    return false;
  }
  if(version!=nullptr) *version=device_config_id;
  
  // 3. use the device config id to get the device configuration
  return GetDeviceConfig(json_data, device_config_id, name, timeout/2);
  
}

// second convenience wrapper
bool Services::GetRunDeviceConfig(std::string& json_data, const std::string& runconfig_name, const int runconfig_version, const std::string& device, int* version, unsigned int timeout){
  
  json_data="";
  
  const std::string& name = (device=="") ? m_name : device;
  
  // 1. get the run configuration
  std::string run_config="";
  bool get_ok = GetRunConfig(run_config, runconfig_name, runconfig_version, timeout/2);
  
  if(!get_ok){
    // redundant as GetRunConfig prints the error - but is this still useful as calling context?
    // TODO we should be using exceptions, of course
    //std::clog<<"GetRunDeviceConfig error getting run config '"<<runconfig_name<<"' version "
    //         <<runconfig_version<<": '"<<run_config<<"'"<<std::endl;
    json_data = run_config;
    return false;
  }
  
  // 2. extract the device's configuration id
  Store tmp;
  tmp.JsonParser(run_config);
  int device_config_id;
  get_ok = tmp.Get(name, device_config_id);
  
  if(!get_ok){
    std::string err= "GetRunDeviceConfig error getting device config; device '"+name
                   +"' not found in run configuration '"+run_config+"'";
    std::clog<<err<<std::endl;
    json_data = err;
    return false;
  }
  if(version!=nullptr) *version=device_config_id;
  
  // 3. use the device config id to get the device configuration
  return GetDeviceConfig(json_data, device_config_id, name, timeout/2);
  
}

// end convenience wrappers

bool Services::GetROOTplot(const std::string& plot_name, int& version, std::string& draw_options, std::string& json_data, std::string* timestamp, const unsigned int timeout){
  
  std::string cmd_string = "{ \"plot_name\":\""+ plot_name + "\""
                         + ", \"version\":" + std::to_string(version)+ "}";
  
  std::string err="";
  std::string response="";
  if(!m_backend_client.SendCommand("R_ROOTPLOT", cmd_string, &response, &timeout, &err)){
    std::clog<<"GetROOTplot error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(response.empty()){
    std::clog<<"GetROOTplot error: empty response, "<<err<<std::endl;
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
    std::clog<<"GetROOTplot error: failed to parse response '"<<response<<"'"<<std::endl;
    json_data = "Parse error";
    return false;
  }
  
  /*
  std::clog<<"timestamp: "<<timestamp<<"\n"
           <<"draw opts: "<<draw_options<<"\n"
           <<"json data: '"<<json_data<<"'"<<std::endl;
  */
  
  return true;
  
}

bool Services::SQLQuery(const std::string& database, const std::string& query, std::vector<std::string>& responses, const unsigned int timeout){
  
  responses.clear();
  
  const std::string& db = (database=="") ? m_dbname : database;
  
  const std::string command = "{ \"database\":\""+db+"\""
                            + ", \"query\":\""+ query+"\" }";
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_QUERY", command, &responses, &timeout, &err)){
    std::clog<<"SQLQuery error: "<<err<<std::endl;
    responses.resize(1);
    responses.front() = err;
    return false;
  }
  
  return true;
  
}

bool Services::SQLQuery(const std::string& database, const std::string& query, std::string& response, const unsigned int timeout){
  
  response="";
  
  const std::string& db = (database=="") ? m_dbname : database;
  
  std::vector<std::string> responses;
  
  bool ok = SQLQuery(db, query, responses, timeout);
  
  if(responses.size()!=0){
    response = responses.front();
    if(responses.size()>1){
      std::clog<<"Warning: SQLQuery returned multiple rows, only first returned"<<std::endl;
    }
  }
  
  return ok;
}

// for things like insertions, the user may not have any return they care about
bool Services::SQLQuery(const std::string& database, const std::string& query, const unsigned int timeout){
  
  std::string tmp;
  return SQLQuery(database, query, tmp, timeout);

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
    std::clog<<"Logging message is too long! Maximum length may be 655355 bytes"<<std::endl;
    return false;
  }
  
  std::string err="";
  
  if(!m_backend_client.SendMulticast(cmd_string, &err)){
    std::clog<<"SendLog error: "<<err<<std::endl;
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
    std::clog<<"Monitoring message is too long! Maximum length may be 655355 bytes"<<std::endl;
    return false;
  }
  
  std::string err="";
  
  if(!m_backend_client.SendMulticast(cmd_string, &err)){
    std::clog<<"SendMonitoringData error: "<<err<<std::endl;
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
    std::clog<<"SendROOTplot error: "<<err<<std::endl;
    return false;
  }
  
  // response is json with the version number of the created plot entry
  // e.g. '{"version":"3"}'. check this is what we got, as validation.
  if(response.length()>14){
    // FIXME change to Store parsing so we can check this is the right key
    response.replace(0,response.find_first_of(':')+1,"");
    response.replace(response.find_last_of('}'),std::string::npos,"");
    try {
      if(version) *version = std::stoi(response);
    } catch (...){
      std::clog<<"SendROOTplot error: invalid response '"<<response<<"'"<<std::endl;
      return false;
    }
  } else {
    std::clog<<"SendROOTplot error: invalid response: '"<<response<<"'"<<std::endl;
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
    std::clog<<"ROOT plot json is too long! Maximum length may be 655355 bytes"<<std::endl;
    return false;
  }
  
  std::string err="";
  
  if(!m_backend_client.SendMulticast(cmd_string, &err)){
    std::clog<<"SendROOTplot error: "<<err<<std::endl;
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
    std::clog << "GetPlot error: " << err << std::endl;
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
    std::clog << "SendPlot error: " << err << std::endl;
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
