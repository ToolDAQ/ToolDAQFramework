#include <Services.h>

using namespace ToolFramework;

namespace {
  constexpr uint32_t MAX_UDP_PACKET_SIZE = 655355;
  constexpr size_t MAX_MSG_SIZE = MAX_UDP_PACKET_SIZE-100; // 100 chars for JSON keys, timestamp string and quotes/commas
}

Services::Services(){
  m_context=0;
  m_name="";
  m_verbose=false;

}
 
Services::~Services(){
  
  m_backend_client.Finalise();
  sc_vars->Stop();
  m_utils.KillThread(&thread_args);
  m_context=nullptr;
  
}

void Services::SetVerbose(bool in){
  m_verbose=in;
}

bool Services::Init(Store &m_variables, zmq::context_t* context_in, SlowControlCollection* sc_vars_in, bool new_service){

  m_context = context_in;
  sc_vars = sc_vars_in;

  bool alerts_send = 0;
  int alert_send_port = 12242;
  bool alerts_receive = 1; 
  int alert_receive_port = 12243;
  int sc_port = 60000;
  mon_merge_period_ms = 1000;
  multicast_send_period_ms = 5000;

  m_variables.Get("alerts_send", alerts_send);
  m_variables.Get("alert_send_port", alert_send_port);
  m_variables.Get("alerts_receive", alerts_receive);
  m_variables.Get("alert_receive_port", alert_receive_port);
  m_variables.Get("sc_port", sc_port);
  m_variables.Get("mon_merge_period_ms",mon_merge_period_ms);
  m_variables.Get("multicast_send_period_ms",multicast_send_period_ms);
  
  sc_vars->InitThreadedReceiver(m_context, sc_port, 100, new_service, alert_receive_port, alerts_receive, alert_send_port, alerts_send);
  m_backend_client.SetUp(m_context);

  sc_vars->Add("State",SlowControlElementType(INFO),0,0,false,false);
  (*sc_vars)["State"]->SetValue(0);
  sc_vars->Add("NewConfig",SlowControlElementType(INFO),0,0,false,false);
  (*sc_vars)["NewConfig"]->SetValue(0);
  
  sc_vars->Add("LoadConfig",SlowControlElementType(VARIABLE),std::bind(&Services::LoadConfig1, this, std::placeholders::_1),0,false,false);
  AlertSubscribe("LoadConfig", std::bind(&Services::LoadConfig2, this,  std::placeholders::_1, std::placeholders::_2));
  
  if(!m_variables.Get("service_name",m_name)) m_name="test_service";
  if(m_name[0]=='('){
    if(m_verbose) std::cerr<<"device names cannot start with '('"<<std::endl;
    return false;
  }

  
  if(!m_backend_client.Initialise(m_variables)){
    if(m_verbose) std::cerr<<"error initialising slowcontrol client"<<std::endl;
    return false;
  }
   
  // wait for the middleman to connect. ServiceDiscovery broadcasts are sent out intermittently,
  // so we need to wait for the middleman to receive one & connect before we can communicate with it.
  int pub_period=5;
  m_variables.Get("service_publish_sec",pub_period);
  if(!Ready(pub_period*3000)){ // Wait up to 3 broadcast periods. It'll return sooner if it connects.
    if(m_verbose) std::cerr<<"Warning: service not yet connected..."<<std::endl;
  }
  
  // start background thread to handle sending batches of buffered logging/monitoring messages
  thread_args.services = this;
  thread_args.logging_buf = &logging_buf;
  thread_args.monitoring_buf = &monitoring_buf;
  thread_args.logging_buf_mtx = &logging_buf_mtx;
  thread_args.monitoring_buf_mtx = &monitoring_buf_mtx;
  thread_args.multicast_send_period_ms = std::chrono::milliseconds{multicast_send_period_ms};
  thread_args.last_send = std::chrono::steady_clock::now();
  if(!m_utils.CreateThread("Services", &BufferThread, &thread_args)){
    if(m_verbose) std::cerr<<"failed to spawn background thread"<<std::endl;
    return false;
  }
  
  return true;
}

bool Services::Ready(const unsigned int timeout){
  return m_backend_client.Ready(timeout);
}

// ===========================================================================
// Write Functions
// ---------------

bool Services::SendAlarm(const std::string& message, bool critical, const std::string& device, const uint64_t timestamp, const unsigned int timeout){
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = "{\"time\":\""+TimeStringFromUnixMs(timestamp)+"\""
                         + ",\"device\":\""+name+"\""
                         + ",\"critical\":"+std::to_string(critical)
                         + ",\"alarm\":\"" + message + "\"}";
  
  std::string err="";
  
  // send the alarm on the pub socket
  bool ok = m_backend_client.SendCommand("W_ALARM", cmd_string, (std::vector<std::string>*)nullptr, &timeout, &err);
  if(!ok){
    if(m_verbose) std::cerr<<"SendAlarm error: "<<err<<std::endl;
  }
  // SendAlarm returns nothing
  
  // also record it to the logging socket
  cmd_string = std::string{"{\"topic\":\"LOGGING\""}
             + ",\"time\":\""+TimeStringFromUnixMs(timestamp)+"\""
             + ",\"device\":\""+name+"\""
             + ",\"severity\":0"
             + ",\"message\":\"" + message + "\"}";
  
  if(!m_backend_client.SendMulticast(MulticastType::Log,cmd_string, &err)){
    if(m_verbose) std::cerr<<"SendAlarm (log) error: "<<err<<std::endl;
  }
  
  return ok;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool Services::SendCalibrationData(const std::string& json_data, const std::string& description, const std::string& name, const uint64_t timestamp, int* version, const unsigned int timeout){
  
  const std::string& c_name = (name=="") ? m_name : name;
  
  std::string cmd_string = "{ \"time\":\""+TimeStringFromUnixMs(timestamp)+"\""
                         + ", \"name\":\""+ c_name +"\""
                         + ", \"description\":\""+ description+"\""
                         + ", \"data\":"+ json_data +" }";
  
  std::string err="";
  std::string response="";
  
  if(!m_backend_client.SendCommand("W_CALIBRATION", cmd_string, &response, &timeout, &err)){
    if(m_verbose) std::cerr<<"SendCalibrationData error: "<<err<<std::endl;
    return false;
  }
  
  if(response.empty()){
    if(m_verbose) std::cerr<<"SendCalibrationData error: empty response"<<std::endl;
    return false;
  }
  
  // response is the version number of the created config entry
  if(version){
    try {
      *version = stoull(response);
    } catch(std::exception& e){
      if(m_verbose) std::cerr<<"SendCalibrationData error: invalid response: '"<<response<<"'"<<std::endl;
      return false;
    }
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool Services::SendDeviceConfig(const std::string& json_data, const std::string& author, const std::string& description, const std::string& device, const uint64_t timestamp, int* version, const unsigned int timeout){
  
  if(version) *version=-1;
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = "{ \"time\":\""+TimeStringFromUnixMs(timestamp)+"\""
                         + ", \"device\":\""+ name+"\""
                         + ", \"author\":\""+ author+"\""
                         + ", \"description\":\""+ description+"\""
                         + ", \"data\":"+ json_data +" }";
  
  std::string response="";
  std::string err="";
  
  if(!m_backend_client.SendCommand("W_DEVCONFIG", cmd_string, &response, &timeout, &err)){
    if(m_verbose) std::cerr<<"SendDeviceConfig error: "<<err<<std::endl;
    return false;
  }
  
  if(response.empty()){
    if(m_verbose) std::cerr<<"SendDeviceConfig error: empty response"<<std::endl;
    return false;
  }
  
  // response is the version number of the created config entry
  if(version){
    try {
      *version = stoull(response);
    } catch(std::exception& e){
      if(m_verbose) std::cerr<<"SendDeviceConfig error: invalid response: '"<<response<<"'"<<std::endl;
      return false;
    }
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool Services::SendBaseConfig(const std::string& json_data, const std::string& name, const std::string& author, const std::string& description, const uint64_t timestamp, int* version, const unsigned int timeout){
  
  if(version) *version=-1;
  
  std::string cmd_string = "{ \"time\":\""+TimeStringFromUnixMs(timestamp)+"\""
                         + ", \"name\":\""+ name+"\""
                         + ", \"author\":\""+ author+"\""
                         + ", \"description\":\""+ description+"\""
                         + ", \"data\":"+ json_data +" }";
  
  std::string response="";
  std::string err="";
  
  if(!m_backend_client.SendCommand("W_BASECONFIG", cmd_string, &response, &timeout, &err)){
    if(m_verbose) std::cerr<<"SendBaseConfig error: "<<err<<std::endl;
    return false;
  }
  
  if(response.empty()){
    if(m_verbose) std::cerr<<"SendBaseConfig error: empty response"<<std::endl;
    return false;
  }
  
  // response is the version number of the created config entry
  if(version){
    try {
      *version = stoull(response);
    } catch(std::exception& e){
      if(m_verbose) std::cerr<<"SendBaseConfig error: invalid response: '"<<response<<"'"<<std::endl;
      return false;
    }
  }
  
  return true;
  
}


// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool Services::SendRunModeConfig(const std::string& json_data, const std::string& name, const std::string& author, const std::string& description, const uint64_t timestamp, int* version, const unsigned int timeout){
  
  if(version) *version=-1;
  
  std::string cmd_string = "{ \"time\":\""+TimeStringFromUnixMs(timestamp)+"\""
                         + ", \"name\":\""+ name+"\""
                         + ", \"author\":\""+ author+"\""
                         + ", \"description\":\""+ description+"\""
                         + ", \"data\":"+ json_data +" }";
  
  std::string response="";
  std::string err="";
  
  if(!m_backend_client.SendCommand("W_RUNMODECONFIG", cmd_string, &response, &timeout, &err)){
    if(m_verbose) std::cerr<<"SendRunModeConfig error: "<<err<<std::endl;
    return false;
  }
  
  if(response.empty()){
    if(m_verbose) std::cerr<<"SendRunModeConfig error: empty response"<<std::endl;
    return false;
  }
  
  // response is the version number of the created config entry
  if(version){
    try {
      *version = stoull(response);
    } catch(std::exception& e){
      if(m_verbose) std::cerr<<"SendRunModeConfig error: invalid response: '"<<response<<"'"<<std::endl;
      return false;
    }
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool Services::SendROOTplot(const std::string& plot_name, const std::string& draw_options, const std::string& json_data, int* version, const uint64_t timestamp, const unsigned int lifetime, const unsigned int timeout){
  
  std::string cmd_string = "{ \"time\":\""+TimeStringFromUnixMs(timestamp)+"\""
                         + ", \"name\":\""+ plot_name +"\""
                         + ", \"draw_options\":\""+ draw_options +"\""
                         + ", \"lifetime\":"+std::to_string(lifetime)
                         + ", \"data\":"+ json_data+" }";
  
  std::string err="";
  std::string response="";
  
  if(!m_backend_client.SendCommand("W_TROOTPLOT", cmd_string, &response, &timeout, &err)){
    if(m_verbose) std::cerr<<"SendROOTplot error: "<<err<<std::endl;
    return false;
  }
  
  if(response.empty()){
    if(m_verbose) std::cerr<<"SendROOTplot error: empty response"<<std::endl;
    return false;
  }
  
  // response is the version number of the created config entry
  if(version){
    try {
      *version = stoull(response);
    } catch(std::exception& e){
      if(m_verbose) std::cerr<<"SendROOTplot error: invalid response: '"<<response<<"'"<<std::endl;
      return false;
    }
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

// single trace version
bool Services::SendPlotlyPlot(
    const std::string& name,
    const std::string& trace,
    const std::string& layout,
    int*               version,
    const uint64_t     timestamp,
    const unsigned int lifetime,
    const unsigned int timeout
) {
  return SendPlotlyPlot(
      name,
      std::vector<std::string> { trace },
      layout,
      version,
      timestamp,
      lifetime,
      timeout
  );
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

// multiple traces version
bool Services::SendPlotlyPlot(
    const std::string&              name,
    const std::vector<std::string>& traces,
    const std::string&              layout,
    int*                            version,
    const uint64_t                  timestamp,
    const unsigned int              lifetime,
    const unsigned int              timeout
) {
  std::stringstream ss;
  ss << "{   \"name\":\"" << name
     << "\", \"time\":\"" << TimeStringFromUnixMs(timestamp)
     << "\", \"lifetime\":" << lifetime
     << ",   \"layout\":" << layout
     << ",   \"data\":[";
  bool first = true;
  for (auto& trace : traces) {
    if (!first) ss << ',';
    first = false;
    ss << trace;
  };
  ss << "]}";
  
  std::string response="";
  
  std::string err;
  if (!m_backend_client.SendCommand("W_PLOTLYPLOT", ss.str(), &response, &timeout, &err)){
    if(m_verbose) std::cerr << "SendPlotlyPlot error: " << err << std::endl;
    return false;
  };
  
  if(response.empty()){
    if(m_verbose) std::cerr<<"SendPlotlyPlot error: empty response"<<std::endl;
    return false;
  }
  
  // response is the version number of the created config entry
  if(version){
    try {
      *version = stoull(response);
    } catch(std::exception& e){
      if(m_verbose) std::cerr<<"SendPlotlyPlot error: invalid response: '"<<response<<"'"<<std::endl;
      return false;
    }
  }
  
  return true;
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

// Since we don't know if a generic SQL query is read or write, call it a write-query for safety
// version that expects multiple returned rows
bool Services::SQLQuery(const std::string& query, std::vector<std::string>& responses, const unsigned int timeout){
  
  responses.clear();
  
  std::string err="";
  
  // for now, commands must not begin with '(' as this is used to identify compressed messages.
  // Since we don't know what a user-provided query string may be, prepend with a space to ensure this.
  std::string sanitized_query = std::string{" "}+query;
  
  if(!m_backend_client.SendCommand("W_QUERY", sanitized_query, &responses, &timeout, &err)){
    if(m_verbose) std::cerr<<"SQLQuery error: "<<err<<std::endl;
    responses.resize(1);
    responses.front() = err;
    return false;
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

// version when expecting just one row
bool Services::SQLQuery(const std::string& query, std::string& response, const unsigned int timeout){
  
  response="";
  
  std::vector<std::string> responses;
  
  bool ok = SQLQuery(query, responses, timeout);
  
  if(responses.size()!=0){
    response = responses.front();
    if(responses.size()>1){
      if(m_verbose) std::cout<<"Warning: SQLQuery returned multiple rows, only first returned"<<std::endl;
    }
  }
  
  return ok;
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

// versions that don't expect any return (e.g. insertions)
bool Services::SQLQuery(const std::string& query, const unsigned int timeout){
  
  std::string tmp;
  return SQLQuery(query, tmp, timeout);

}

// ===========================================================================
// Read Functions
// --------------
// we need to ensure any escaping needed is already done, but probably none needed

bool Services::GetCalibrationData(std::string& json_data, int& version, const std::string& device, const unsigned int timeout){
  
  json_data = "";
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string;
  
  if(version<0){
    // https://stackoverflow.com/questions/tagged/greatest-n-per-group for faster
    cmd_string = "SELECT json_build_object('data', data, 'version', version) FROM calibration WHERE name='"+name+"' ORDER BY version DESC LIMIT 1";
  } else {
    cmd_string = "SELECT json_build_object('data', data, 'version', version) FROM calibration WHERE device='"+name+"' AND version="+std::to_string(version);
  }
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_CALIBRATION", cmd_string, &json_data, &timeout, &err)){
    if(m_verbose) std::cerr<<"GetCalibrationData error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(json_data.empty()){
    // if we got an empty response but the command succeeded,
    // the query worked but matched no records - run config not found
    err = "GetCalibrationData error: data for device "+name+" version "+std::to_string(version)+" not found";
    if(m_verbose) std::cerr<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  Store tmp;
  tmp.JsonParser(json_data);
  bool ok = tmp.Get("data",json_data);
  ok = ok && tmp.Get("version",version);
  
  if(!ok){
    err = "GetCalibrationData error: invalid response: '"+json_data+"'";
    if(m_verbose) std::cerr<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  
  return true;
  
}

bool Services::GetCalibrationData(std::string& json_data, int&& version, const std::string& device, const unsigned int timeout){
  int v = version;
  return GetCalibrationData(json_data, v, device, timeout);
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool Services::GetDeviceConfig(std::string& json_data, const int version, const std::string& device, const unsigned int timeout){

  json_data="";
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string;
  if(version<0){
    // https://stackoverflow.com/questions/tagged/greatest-n-per-group for faster
    cmd_string = "SELECT json_build_object('data', data) FROM device_config WHERE device='"+name+"' ORDER BY version DESC LIMIT 1";
  } else {
  cmd_string = "SELECT json_build_object('data', data) FROM device_config WHERE device='"+name+"' AND version="+std::to_string(version);
  }
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_DEVCONFIG", cmd_string, &json_data, &timeout, &err)){
    if(m_verbose) std::cerr<<"GetDeviceConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(json_data.empty()){
    // if we got an empty response but the command succeeded,
    // the query worked but matched no records - run config not found
    err = "GetDeviceConfig error: config for device "+name+" version "+std::to_string(version)+" not found";
    if(m_verbose) std::cerr<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  // response format '{"data":"<contents>"}' - strip out contents
  Store tmp;
  tmp.JsonParser(json_data);
  bool ok = tmp.Get("data", json_data);
  if(!ok){
    err = "GetDeviceConfig error: invalid response: '"+json_data+"'";
    if(m_verbose) std::cerr<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

// get a run configuration via configuration ID pair
bool Services::GetRunConfig(std::string& json_data, const int base_config_id, const int runmode_config_id, const unsigned int timeout){
  
  json_data="";
  
  std::string cmd_string = "SELECT json_build_object('data', base.data || runmode.data) FROM ( SELECT data FROM base_config WHERE config_id="
                         + std::to_string(base_config_id) + ") base CROSS JOIN (SELECT data FROM runmode_config WHERE config_id="
                         + std::to_string(runmode_config_id) + ") runmode";
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_RUNCONFIG", cmd_string, &json_data, &timeout, &err)){
    if(m_verbose) std::cerr<<"GetRunConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(json_data.empty()){
    // if we got an empty response but the command succeeded,
    // the query worked but matched no records - run config not found
    err = "GetRunConfig error: no config matching base config "+std::to_string(base_config_id)+", runmode config "+std::to_string(runmode_config_id);
    if(m_verbose) std::cerr<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  // response format '{"data":<contents>}' - strip out contents
  Store tmp;
  tmp.JsonParser(json_data);
  bool ok = tmp.Get("data", json_data);
  if(!ok){
    err="GetRunConfig error: invalid response: '"+json_data+"'";
    if(m_verbose) std::cerr<<err<<std::endl;
    json_data=err;
    return false;
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

// get a run configuration by name and version (e.g. name: AmBe, version: 3)
bool Services::GetRunModeConfig(std::string& json_data, const std::string& name, const int version, const unsigned int timeout){

  json_data="";
  
  std::string cmd_string;
  
  if(version<0){
    cmd_string = "SELECT json_build_object('data', data, 'version', version) FROM runmode_config WHERE name='"+name+"' ORDER BY version DESC LIMIT 1";
  } else {
    cmd_string = "SELECT json_build_object('data', data, 'version', version) FROM runmode_config WHERE name='"+name+"' AND version="+std::to_string(version);
  }
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_RUNCONFIG", cmd_string, &json_data, &timeout, &err)){
    if(m_verbose) std::cerr<<"GetRunConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(json_data.empty()){
    // if we got an empty response but the command succeeded,
    // the query worked but matched no records - run config not found
    err = "GetRunConfig error: config "+name+" version "+std::to_string(version)+" not found";
    if(m_verbose) std::cerr<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  // response format '{"data":"<contents>"}' - strip out contents
  Store tmp;
  tmp.JsonParser(json_data);
  bool ok = tmp.Get("data", json_data);
  if(!ok){
    err="GetRunConfig error: invalid response: '"+json_data+"'";
    if(m_verbose) std::cerr<<err<<std::endl;
    json_data=err;
    return false;
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

// Get a device configuration from a *run* configuration ID
bool Services::GetRunDeviceConfig(std::string& json_data, const int base_config_id, const int runmode_config_id, const std::string& device, int* version, unsigned int timeout){
  
  json_data="";
  
  const std::string& name = (device=="") ? m_name : device;
  
  /*
  std::string cmd_string = "{ \"base_config_id\":"+std::to_string(base_config_id)
                         + ", \"runmode_config_id\":"+std::to_string(runmode_config_id)
                         + ", \"device\":\""+name+"\"}";
  */
  
  std::string cmd_string = "WITH base AS ( SELECT data FROM base_config WHERE config_id="+std::to_string(base_config_id)+"), "
                           "  runmode AS ( SELECT data FROM runmode_config WHERE config_id="+std::to_string(runmode_config_id)+") "
                           "SELECT json_build_object('version', version, 'data', data) FROM device_config WHERE device='"+name+"' "
                           "AND version=(SELECT ((base.data || runmode.data)->'"+name+"')::integer FROM base CROSS JOIN runmode)";
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_RUNDEVICECONFIG", cmd_string, &json_data, &timeout, &err)){
    if(m_verbose) std::cerr<<"GetRunDeviceConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(json_data.empty()){
    // if we got an empty response but the command succeeded,
    // the query worked but matched no records - run config not found
    err = "GetRunDeviceConfig error: config "+name+" for base config "+std::to_string(base_config_id)+" and runmode config "+std::to_string(runmode_config_id)+" not found";
    if(m_verbose) std::cerr<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  // response format '{"version": X, "data":"<contents>"}' - strip out contents
  Store tmp;
  tmp.JsonParser(json_data);
  int tmp_version;
  bool ok = tmp.Get("version",tmp_version);
  if(ok && version){
    *version = tmp_version;
  }
  ok = ok && tmp.Get("data", json_data);
  if(!ok){
    err="GetRunDeviceConfig error: invalid response: '"+json_data+"'";
    if(m_verbose) std::cerr<<err<<std::endl;
    json_data=err;
    return false;
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

// Get a device configuration from a *run* configuration ID
bool Services::GetCachedDeviceConfig(std::string& json_data, int base_config_id, int runmode_config_id, const std::string& device, int* version, unsigned int timeout){
  
  json_data="";
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_KACHEDDEVICECONFIG", name, &json_data, &timeout, &err)){
    if(m_verbose) std::cerr<<"GetRunDeviceConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(json_data.empty()){
    // if we got an empty response but the command succeeded, device name was not found in map
    err = "GetCachedDeviceConfig error: config for device "+name+" not found";
    if(m_verbose) std::cerr<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  // response format '{"base_config_id":N, "runmode_config_id":M, version": X, "data":"<contents>"}' - strip out contents
  Store tmp;
  tmp.JsonParser(json_data);
  int base=-1, runmode=-1;
  tmp.Get("base_config_id",base);
  tmp.Get("runmode_config_id",runmode);
  if(base!=base_config_id || runmode!=runmode_config_id){
    err="GetCachedDeviceConfig returned unexpected configuration ids: {"+std::to_string(base)+","+std::to_string(runmode)
       +"}, expected {"+std::to_string(base_config_id)+","+std::to_string(runmode_config_id)+"}";
    if(m_verbose) std::cerr<<err<<std::endl;
    json_data=err;
    return false;
  }
  
  int tmp_version;
  bool ok = tmp.Get("version",tmp_version);
  if(ok && version){
    *version = tmp_version;
  }
  ok = ok && tmp.Get("data", json_data);
  if(!ok){
    err="GetRunDeviceConfig error: invalid response: '"+json_data+"'";
    if(m_verbose) std::cerr<<err<<std::endl;
    json_data=err;
    return false;
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

/* doesn't make sense any more as we need to also specify a base config number to guarantee it returns a configuration
// Get a device configuration from a *run* type name and version number
bool Services::GetRunDeviceConfig(std::string& json_data, const std::string& runconfig_name, const int runconfig_version, const std::string& device, int* version, unsigned int timeout){
  
  json_data="";
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = "SELECT json_build_object('data', data, 'version', version) FROM device_config WHERE device='"+name+"' AND version=(SELECT data->>'"+name+"' FROM run_config WHERE name='"+runconfig_name+"' AND version="+std::to_string(runconfig_version)+")::integer";
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_DEVCONFIG", cmd_string, &json_data, &timeout, &err)){
    if(m_verbose) std::cerr<<"GetRunDeviceConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(json_data.empty()){
    // if we got an empty response but the command succeeded,
    // the query worked but matched no records - run config not found
    err = "GetRunDeviceConfig error: config "+name+" for runconfig "+runconfig_name+" version "+std::to_string(runconfig_version)+" not found";
    if(m_verbose) std::cerr<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  // response format '{"version":X, "data":"<contents>"}' - strip out contents
  Store tmp;
  tmp.JsonParser(json_data);
  int tmp_version;
  bool ok = tmp.Get("version",tmp_version);
  if(ok && version){
    *version = tmp_version;
  }
  ok = ok && tmp.Get("data", json_data);
  if(!ok){
    err="GetRunDeviceConfig error: invalid response: '"+json_data+"'";
    if(m_verbose) std::cerr<<err<<std::endl;
    json_data=err;
    return false;
  }
  
  return true;
  
}
*/

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool Services::GetROOTplot(const std::string& plot_name, std::string& draw_options, std::string& json_data, int& version, const unsigned int timeout){
  
  std::string cmd_string;
  
  if(version<0){
    // https://stackoverflow.com/questions/tagged/greatest-n-per-group for faster
    cmd_string = "SELECT json_build_object('version', version, 'data', data, 'draw_options', draw_options) FROM rootplots WHERE name='"+plot_name+"' ORDER BY version DESC LIMIT 1";
  } else {
    cmd_string = "SELECT json_build_object('version', version, 'data', data, 'draw_options', draw_options) FROM rootplots WHERE name='"+plot_name+"' AND version="+std::to_string(version);
  }
  
  std::string err="";
  std::string response="";
  
  if(!m_backend_client.SendCommand("R_TROOTPLOT", cmd_string, &response, &timeout, &err)){
    if(m_verbose) std::cerr<<"GetROOTplot error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(response.empty()){
    err = "GetROOTplot error: ROOTplot "+plot_name+" version "+std::to_string(version)+" not found";
    json_data = err;
    return false;
  }
  
  Store plot;
  plot.JsonParser(response);
  
  bool ok = plot.Get("data", json_data);
  ok = ok && plot.Get("draw_options", draw_options);
  ok = ok && plot.Get("version", version);
  
  if(!ok){
    err="GetROOTplot error: invalid response: '"+response+"'";
    if(m_verbose) std::cerr<<err<<std::endl;
    json_data=err;
    return false;
  }
  
  /*
  std::clog<<"draw opts: "<<draw_options<<"\n"
           <<"json data: '"<<json_data<<"'"<<std::endl;
  */
  
  return true;
  
}

// version that accepts values in case user isn't interested in returned version
bool Services::GetROOTplot(const std::string& plot_name, std::string& draw_options, std::string& json_data, int&& version, const unsigned int timeout){
  int v = version;
  return GetROOTplot(plot_name, draw_options, json_data, v, timeout);
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool Services::GetPlotlyPlot(
    const std::string& name,
    std::string&       trace,
    std::string&       layout,
    int&               version,
    unsigned int       timeout
) {
  std::string cmd_string;
  if(version<0){
    // https://stackoverflow.com/questions/tagged/greatest-n-per-group for faster
    cmd_string = "SELECT json_build_object('version', version, 'data', data, 'layout', layout) FROM plotlyplots WHERE name='"+name+"' ORDER BY version DESC LIMIT 1";
  } else {
    cmd_string = "SELECT json_build_object('version', version, 'data', data, 'layout', layout) FROM plotlyplots WHERE name='"+name+"' AND version="+std::to_string(version);
  }
  
  std::string err;
  std::string response;
  if (!m_backend_client.SendCommand("R_PLOTLYPLOT", cmd_string, &response, &timeout, &err)){
    if(m_verbose) std::cerr << "GetPlotlyPlot error: " << err << std::endl;
    trace = err;
    return false;
  };
  
  if(response.empty()){
    err = "GetPlotlyPlot error: PlotlyPlot "+name+" version "+std::to_string(version)+" not found";
    trace = err;
    return false;
  }
  
  Store plot;
  plot.JsonParser(response);
  
  bool ok = plot.Get("data", trace);
  ok = ok && plot.Get("layout", layout);
  ok = ok && plot.Get("version", version);
  
  if(!ok){
    err="GetPlotlyPlot error: invalid response: '"+response+"'";
    if(m_verbose) std::cerr<<err<<std::endl;
    trace=err;
    return false;
  }
  
  return true;
}

bool Services::GetPlotlyPlot(const std::string& name, std::string& trace, std::string& layout, int&& version, unsigned int timeout){
  int v = version;
  return GetPlotlyPlot(name, trace, layout, v, timeout);
} 

// ===========================================================================
// Multicast Senders
// -----------------

bool Services::SendLog(const std::string& message, LogLevel severity, const std::string& device, const uint64_t timestamp){
  
  const std::string& name = (device=="") ? m_name : device;
  
  // FIXME we should be able to relax this check if compression is enabled...
  if((message.length()+name.length())>MAX_MSG_SIZE){
    if(m_verbose) std::cerr<<"Logging message is too long!"<<std::endl;
    return false;
  }
  
  std::unique_lock<std::mutex> locker(logging_buf_mtx);
  
  if(logging_buf.size() && name==logging_buf.back().device && message==logging_buf.back().message){
    ++logging_buf.back().repeats;
    return true;
  }
  
  // grab timestamp at time of call if 0
  time_t ts = (timestamp!=0) ? timestamp : time(nullptr)*1000;
  
  logging_buf.emplace_back(message, severity, name, ts);
  
  return true;
}

bool Services::SendLog(std::string& msg){
  
  std::string err="";
  
  if(!m_backend_client.SendMulticast(MulticastType::Log, msg, &err)){
    if(m_verbose) std::cerr<<"SendLog error: "<<err<<std::endl;
    return false;
  }
  
  return true;
  
}


bool Services::SendMonitoringData(const std::string& json_data, const std::string& subject, const std::string& device, const uint64_t timestamp){
  
  const std::string& name = (device=="") ? m_name : device;
  
  if((json_data.length()+name.length()+subject.length())>MAX_MSG_SIZE){
    if(m_verbose) std::cerr<<"Monitoring message is too long!"<<std::endl;
    return false;
  }
  
  // grab timestamp at time of call if 0
  time_t ts = (timestamp!=0) ? timestamp : time(nullptr)*1000;
  
  std::unique_lock<std::mutex> locker(monitoring_buf_mtx);
  
  // take first of repeated monitoring sends within buffer period
  auto it = monitoring_buf.find(name+subject);
  if(it!=monitoring_buf.end() && (ts - it->second.timestamp)<mon_merge_period_ms) return true;
  
  monitoring_buf.emplace(std::piecewise_construct,
                         std::forward_as_tuple(name+subject),
                         std::forward_as_tuple(json_data, subject, name, ts));
  
  return true;
}

// cmd_string represents a batch of messages
bool Services::SendMonitoringData(std::string& msg){
  
  std::string err="";
  
  if(!m_backend_client.SendMulticast(MulticastType::Monitoring, msg, &err)){
    if(m_verbose) std::cerr<<"SendMonitoringData error: "<<err<<std::endl;
    return false;
  }
  
  return true;
  
}

// send ROOT plot over multicast
bool Services::SendROOTplotMulticast(const std::string& plot_name, const std::string& draw_options, const std::string& json_data, const unsigned int lifetime, const uint64_t timestamp){
  
  std::string cmd_string = std::string{"{\"topic\":\"TROOTPLOT\""}
                         + ", \"time\":\""+TimeStringFromUnixMs(timestamp)+"\""
                         + ", \"name\":\""+ plot_name +"\""
                         + ", \"draw_options\":\""+ draw_options +"\""
                         + ", \"lifetime\":"+std::to_string(lifetime)
                         + ", \"data\":"+ json_data+"}";
  
  if(cmd_string.length()>MAX_UDP_PACKET_SIZE){
    if(m_verbose) std::cerr<<"ROOT plot json is too long! Maximum length may be MAX_UDP_PACKET_SIZE bytes"<<std::endl;
    return false;
  }
  
  std::string err="";
  
  if(!m_backend_client.SendMulticast(MulticastType::Log,cmd_string, &err)){
    if(m_verbose) std::cerr<<"SendROOTplot error: "<<err<<std::endl;
    return false;
  }
  
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

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

std::string Services::TimeStringFromUnixMs(const uint64_t timestamp){
  
  if(timestamp==1) return "now()";  // remotely interpret 'now'
  
  time_t timestamp_sec; // time_t is equivalent to uint64_t
  uint16_t timestamp_ms = 0;
  if(timestamp==0){
    timestamp_sec = time(nullptr)*1000; // locally interpret 'now'
  } else {
    timestamp_ms = timestamp%1000;
    timestamp_sec = timestamp/1000;
  }
  struct tm timestruct;
  gmtime_r(&timestamp_sec, &timestruct); // FIXME error checking?
  char timestring[24];
  int nchars = strftime(&timestring[0], 20, "%F %T", &timestruct);
  if(nchars==0){
    //Log("strftime error converting time struct '"+std::to_string(timestamp)+"' to string",v_error);
    return "now()";
  }
  // add the milliseconds
  nchars = snprintf(&timestring[19], 5, ".%03d", timestamp_ms);
  /*
  if(nchars!=4){
    Log("snprintf error converting '"+std::to_string(timestamp_ms)+"' to timestamp milliseconds",v_error);
    return "now()";  // just omit the milliseconds? or fall back to 'now'?
  }
  */
  
  return std::string{timestring};
  
}

void Services::LoadConfig2(const char* alert, const char* payload){
  
  Store tmp;
  tmp.JsonParser(payload);
  uint64_t base_config_id=0;
  uint64_t run_mode_config_id=0;
  
  tmp.Get("Base",base_config_id);
  tmp.Get("RunMode",run_mode_config_id);
  
  if(run_mode_config_id!=m_run_mode_config_id || base_config_id!=m_base_config_id){
    
    if(!GetCachedDeviceConfig(m_local_config, base_config_id, run_mode_config_id)){
      usleep(100000);
    }
    (*sc_vars)["NewConfig"]->SetValue(1);
    m_base_config_id = base_config_id;
    m_run_mode_config_id = run_mode_config_id;
    
  }

}

std::string Services::LoadConfig1(const char* control){
  
  std::string payload = (*sc_vars)[control]->GetValue<std::string>();
  Store tmp;
  tmp.JsonParser(payload);
  uint64_t base_config_id=0;
  uint64_t run_mode_config_id=0;
  
  tmp.Get("Base",base_config_id);
  tmp.Get("RunMode",run_mode_config_id);
  
  if(run_mode_config_id!=m_run_mode_config_id || base_config_id!=m_base_config_id){
    
    if(!GetRunDeviceConfig(m_local_config, base_config_id, run_mode_config_id)){
      usleep(100000);
    }
    (*sc_vars)["NewConfig"]->SetValue(1);
    m_base_config_id = base_config_id;
    m_run_mode_config_id = run_mode_config_id;
    
  }
  
  return "";
  
}

// ========================

void Services::BufferThread(Thread_args* args){
  
  BufferThreadArgs* m_args = dynamic_cast<BufferThreadArgs*>(args);
  
  m_args->last_send = std::chrono::steady_clock::now();
  
  m_args->local_merge_buf.clear();
  std::unique_lock<std::mutex> locker(*m_args->logging_buf_mtx);
  
  // merge into a batch
  bool first=true;
  for(LogMsg& msg : *m_args->logging_buf){
    m_args->local_merge_buf += std::string(first ? "" : ",")
                            + "{\"topic\":\"LOGGING\""
                            + ",\"time\":\""+TimeStringFromUnixMs(msg.timestamp)+"\""
                            + ",\"device\":\""+ msg.device +"\""
                            + ",\"severity\":"+std::to_string(int(msg.severity))
                            + ",\"message\":\"" + msg. message + "\""
                            + ",\"repeats\":"+std::to_string(msg.repeats)+"}";
    first=false;
  }
  
  // send
  if(m_args->services->SendLog(m_args->local_merge_buf)){
    m_args->logging_buf->clear(); // FIXME do we not clear on error...? does it depend on the error...?
  }
  
  // repeat for monitoring messages
  m_args->local_merge_buf.clear();
  locker = std::unique_lock<std::mutex>(*m_args->monitoring_buf_mtx);
  
  first=true;
  for(std::pair<const std::string, MonitoringMsg>& msg : *m_args->monitoring_buf){
    m_args->local_merge_buf += std::string(first ? "" : ",")
                            + "{\"topic\":\"MONITORING\""
                            + ",\"time\":\""+TimeStringFromUnixMs(msg.second.timestamp)+"\""
                            + ",\"device\":\""+ msg.second.device +"\""
                            + ",\"subject\":\""+ msg.second.subject +"\""
                            + ",\"data\":"+ msg.second.json_data +"}";
    first=false;
  }
  
  // send
  if(m_args->services->SendMonitoringData(m_args->local_merge_buf)){
    m_args->monitoring_buf->clear(); // FIXME do we not clear on error...? does it depend on the error...?
  }
  
  std::this_thread::sleep_until(m_args->last_send+m_args->multicast_send_period_ms);
  
  return;
}
