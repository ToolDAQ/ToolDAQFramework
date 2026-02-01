#include <Services.h>

using namespace ToolFramework;

namespace {
  const uint32_t MAX_UDP_PACKET_SIZE = 655355;
}

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

  bool alerts_send = 0;
  int alert_send_port = 12242;
  bool alerts_receive = 1; 
  int alert_receive_port = 12243;
  int sc_port = 60000;

  m_variables.Get("alerts_send", alerts_send);
  m_variables.Get("alert_send_port", alert_send_port);
  m_variables.Get("alerts_receive", alerts_receive);
  m_variables.Get("alert_receive_port", alert_receive_port);
  m_variables.Get("sc_port", sc_port);
  
  sc_vars->InitThreadedReceiver(m_context, sc_port, 100, new_service, alert_receive_port, alerts_receive, alert_send_port, alerts_send);
  m_backend_client.SetUp(m_context);
  
  if(!m_variables.Get("service_name",m_name)) m_name="test_service";
  if(!m_variables.Get("db_name",m_dbname)) m_dbname="daq";

  
  if(!m_backend_client.Initialise(m_variables)){
    std::cerr<<"error initialising slowcontrol client"<<std::endl;
    return false;
  }
   
  // wait for the middleman to connect. ServiceDiscovery broadcasts are sent out intermittently,
  // so we need to wait for the middleman to receive one & connect before we can communicate with it.
  int pub_period=5;
  m_variables.Get("service_publish_sec",pub_period);
  if(!Ready(pub_period*3000)){ // Wait up to 3 broadcast periods. It'll return sooner if it connects.
    std::cerr<<"Warning: service not yet connected..."<<std::endl;
  }
  
  return true;
}

bool Services::Ready(const unsigned int timeout){
  return m_backend_client.Ready(timeout);
}

// ===========================================================================
// Write Functions
// ---------------

bool Services::SendAlarm(const std::string& message, unsigned int level, const std::string& device, const uint64_t timestamp, const unsigned int timeout){
  
  const std::string& name = (device=="") ? m_name : device;
  
  // we only handle 2 levels of alarm: critical (level 0) and normal (level!=0).
  if(level>0) level=1;
  
  std::string cmd_string = "{\"time\":\""+TimeStringFromUnixMs(timestamp)+"\""
                         + ",\"device\":\""+name+"\""
                         + ",\"level\":"+std::to_string(level)
                         + ",\"alarm\":\"" + message + "\"}";
  
  std::string err="";
  
  // send the alarm on the pub socket
  bool ok = m_backend_client.SendCommand("W_ALARM", cmd_string, (std::vector<std::string>*)nullptr, &timeout, &err);
  if(!ok){
    std::cerr<<"SendAlarm error: "<<err<<std::endl;
  }
  // SendAlarm returns nothing
  
  // also record it to the logging socket
  cmd_string = std::string{"{\"topic\":\"LOGGING\""}
             + ",\"time\":\""+TimeStringFromUnixMs(timestamp)+"\""
             + ",\"device\":\""+name+"\""
             + ",\"severity\":0"
             + ",\"message\":\"" + message + "\"}";
  
  if(!m_backend_client.SendMulticast(MulticastType::Log,cmd_string, &err)){
    std::cerr<<"SendAlarm (log) error: "<<err<<std::endl;
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
    std::cerr<<"SendCalibrationData error: "<<err<<std::endl;
    return false;
  }
  
  if(response.empty()){
    std::cerr<<"SendCalibrationData error: empty response"<<std::endl;
    return false;
  }
  
  // response is the version number of the created config entry
  if(version){
    try {
      *version = stoull(response);
    } catch(std::exception& e){
      std::cerr<<"SendCalibrationData error: invalid response: '"<<response<<"'"<<std::endl;
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
    std::cerr<<"SendDeviceConfig error: "<<err<<std::endl;
    return false;
  }
  
  if(response.empty()){
    std::cerr<<"SendDeviceConfig error: empty response"<<std::endl;
    return false;
  }
  
  // response is the version number of the created config entry
  if(version){
    try {
      *version = stoull(response);
    } catch(std::exception& e){
      std::cerr<<"SendDeviceConfig error: invalid response: '"<<response<<"'"<<std::endl;
      return false;
    }
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool Services::SendRunConfig(const std::string& json_data, const std::string& name, const std::string& author, const std::string& description, const uint64_t timestamp, int* version, const unsigned int timeout){
  
  if(version) *version=-1;
  
  std::string cmd_string = "{ \"time\":\""+TimeStringFromUnixMs(timestamp)+"\""
                         + ", \"name\":\""+ name+"\""
                         + ", \"author\":\""+ author+"\""
                         + ", \"description\":\""+ description+"\""
                         + ", \"data\":"+ json_data +" }";
  
  std::string response="";
  std::string err="";
  
  if(!m_backend_client.SendCommand("W_RUNCONFIG", cmd_string, &response, &timeout, &err)){
    std::cerr<<"SendRunConfig error: "<<err<<std::endl;
    return false;
  }
  
  if(response.empty()){
    std::cerr<<"SendRunConfig error: empty response"<<std::endl;
    return false;
  }
  
  // response is the version number of the created config entry
  if(version){
    try {
      *version = stoull(response);
    } catch(std::exception& e){
      std::cerr<<"SendRunConfig error: invalid response: '"<<response<<"'"<<std::endl;
      return false;
    }
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool Services::SendROOTplotZmq(const std::string& plot_name, const std::string& draw_options, const std::string& json_data, int* version, const uint64_t timestamp, const unsigned int lifetime, const unsigned int timeout){
  
  std::string cmd_string = "{ \"time\":\""+TimeStringFromUnixMs(timestamp)+"\""
                         + ", \"name\":\""+ plot_name +"\""
                         + ", \"draw_options\":\""+ draw_options +"\""
                         + ", \"lifetime\":"+std::to_string(lifetime)
                         + ", \"data\":"+ json_data+" }";
  
  std::string err="";
  std::string response="";
  
  if(!m_backend_client.SendCommand("W_TROOTPLOT", cmd_string, &response, &timeout, &err)){
    std::cerr<<"SendROOTplot error: "<<err<<std::endl;
    return false;
  }
  
  if(response.empty()){
    std::cerr<<"SendROOTplot error: empty response"<<std::endl;
    return false;
  }
  
  // response is the version number of the created config entry
  if(version){
    try {
      *version = stoull(response);
    } catch(std::exception& e){
      std::cerr<<"SendROOTplot error: invalid response: '"<<response<<"'"<<std::endl;
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
    std::cerr << "SendPlotlyPlot error: " << err << std::endl;
    return false;
  };
  
  if(response.empty()){
    std::cerr<<"SendPlotlyPlot error: empty response"<<std::endl;
    return false;
  }
  
  // response is the version number of the created config entry
  if(version){
    try {
      *version = stoull(response);
    } catch(std::exception& e){
      std::cerr<<"SendPlotlyPlot error: invalid response: '"<<response<<"'"<<std::endl;
      return false;
    }
  }
  
  return true;
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

// Since we don't know if a generic SQL query is read or write, call it a write-query for safety
// version that expects multiple returned rows
bool Services::SQLQuery(/*const std::string& database,*/ const std::string& query, std::vector<std::string>& responses, const unsigned int timeout){
  
  responses.clear();
  
  //const std::string& db = (database=="") ? m_dbname : database;
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("W_QUERY", query, &responses, &timeout, &err)){
    std::cerr<<"SQLQuery error: "<<err<<std::endl;
    responses.resize(1);
    responses.front() = err;
    return false;
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

// version when expecting just one row
bool Services::SQLQuery(/*const std::string& database,*/ const std::string& query, std::string& response, const unsigned int timeout){
  
  response="";
  
  std::vector<std::string> responses;
  
  bool ok = SQLQuery(/*db,*/ query, responses, timeout);
  
  if(responses.size()!=0){
    response = responses.front();
    if(responses.size()>1){
      std::cout<<"Warning: SQLQuery returned multiple rows, only first returned"<<std::endl;
    }
  }
  
  return ok;
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

// versions that don't expect any return (e.g. insertions)
bool Services::SQLQuery(/*const std::string& database,*/ const std::string& query, const unsigned int timeout){
  
  std::string tmp;
  return SQLQuery(/*database,*/ query, tmp, timeout);

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
    cmd_string = "SELECT jsonb_build_object('data', data, 'version', version) FROM calibration WHERE name='"+name+"' ORDER BY version DESC LIMIT 1";
  } else {
    cmd_string = "SELECT jsonb_build_object('data', data, 'version', version) FROM calibration WHERE device='"+name+"' AND version="+std::to_string(version);
  }
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_CALIBRATION", cmd_string, &json_data, &timeout, &err)){
    std::cerr<<"GetCalibrationData error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(json_data.empty()){
    // if we got an empty response but the command succeeded,
    // the query worked but matched no records - run config not found
    err = "GetCalibrationData error: data for device "+name+" version "+std::to_string(version)+" not found";
    std::cerr<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  Store tmp;
  tmp.JsonParser(json_data);
  bool ok = tmp.Get("data",json_data);
  ok = ok && tmp.Get("version",version);
  
  if(!ok){
    err = "GetCalibrationData error: invalid response: '"+json_data+"'";
    std::cerr<<err<<std::endl;
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
    cmd_string = "SELECT jsonb_build_object('data', data) FROM device_config WHERE device='"+name+"' ORDER BY version DESC LIMIT 1";
  } else {
  cmd_string = "SELECT jsonb_build_object('data', data) FROM device_config WHERE device='"+name+"' AND version="+std::to_string(version);
  }
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_DEVCONFIG", cmd_string, &json_data, &timeout, &err)){
    std::cerr<<"GetDeviceConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(json_data.empty()){
    // if we got an empty response but the command succeeded,
    // the query worked but matched no records - run config not found
    err = "GetDeviceConfig error: config for device "+name+" version "+std::to_string(version)+" not found";
    std::cerr<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  // response format '{"data":"<contents>"}' - strip out contents
  Store tmp;
  tmp.JsonParser(json_data);
  bool ok = tmp.Get("data", json_data);
  if(!ok){
    err = "GetDeviceConfig error: invalid response: '"+json_data+"'";
    std::cerr<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

// get a run configuration via configuration ID
bool Services::GetRunConfig(std::string& json_data, const int config_id, const unsigned int timeout){

  json_data="";
  
  std::string cmd_string = "SELECT jsonb_build_object('data', data) FROM run_config WHERE config_id="+std::to_string(config_id);
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_RUNCONFIG", cmd_string, &json_data, &timeout, &err)){
    std::cerr<<"GetRunConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(json_data.empty()){
    // if we got an empty response but the command succeeded,
    // the query worked but matched no records - run config not found
    err = "GetRunConfig error: config_id "+std::to_string(config_id)+" not found";
    std::cerr<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  // response format '{"data":<contents>}' - strip out contents
  Store tmp;
  tmp.JsonParser(json_data);
  bool ok = tmp.Get("data", json_data);
  if(!ok){
    err="GetRunConfig error: invalid response: '"+json_data+"'";
    std::cerr<<err<<std::endl;
    json_data=err;
    return false;
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

// get a run configuration by name and version (e.g. name: AmBe, version: 3)
bool Services::GetRunConfig(std::string& json_data, const std::string& name, const int version, const unsigned int timeout){

  json_data="";
  
  std::string cmd_string = "SELECT jsonb_build_object('data', data) FROM run_config WHERE name='"+name+"' AND version="+std::to_string(version);
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_RUNCONFIG", cmd_string, &json_data, &timeout, &err)){
    std::cerr<<"GetRunConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(json_data.empty()){
    // if we got an empty response but the command succeeded,
    // the query worked but matched no records - run config not found
    err = "GetRunConfig error: config "+name+" version "+std::to_string(version)+" not found";
    std::cerr<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  // response format '{"data":"<contents>"}' - strip out contents
  Store tmp;
  tmp.JsonParser(json_data);
  bool ok = tmp.Get("data", json_data);
  if(!ok){
    err="GetRunConfig error: invalid response: '"+json_data+"'";
    std::cerr<<err<<std::endl;
    json_data=err;
    return false;
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

// Get a device configuration from a *run* configuration ID
bool Services::GetRunDeviceConfig(std::string& json_data, const int runconfig_id, const std::string& device, int* version, unsigned int timeout){
  
  json_data="";
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = "SELECT jsonb_build_object('data', data, 'version', version) FROM device_config WHERE device='"+name+"' AND version=(SELECT data->>'"+name+"' FROM run_config WHERE config_id="+std::to_string(runconfig_id)+")::integer";
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_DEVCONFIG", cmd_string, &json_data, &timeout, &err)){
    std::cerr<<"GetRunDeviceConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(json_data.empty()){
    // if we got an empty response but the command succeeded,
    // the query worked but matched no records - run config not found
    err = "GetRunDeviceConfig error: config "+name+" for runconfig "+std::to_string(runconfig_id)+" not found";
    std::cerr<<err<<std::endl;
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
    std::cerr<<err<<std::endl;
    json_data=err;
    return false;
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

// Get a device configuration from a *run* type name and version number
bool Services::GetRunDeviceConfig(std::string& json_data, const std::string& runconfig_name, const int runconfig_version, const std::string& device, int* version, unsigned int timeout){
  
  json_data="";
  
  const std::string& name = (device=="") ? m_name : device;
  
    std::string cmd_string = "SELECT jsonb_build_object('data', data, 'version', version) FROM device_config WHERE device='"+name+"' AND version=(SELECT data->>'"+name+"' FROM run_config WHERE name='"+runconfig_name+"' AND version="+std::to_string(runconfig_version)+")::integer";
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_DEVCONFIG", cmd_string, &json_data, &timeout, &err)){
    std::cerr<<"GetRunDeviceConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(json_data.empty()){
    // if we got an empty response but the command succeeded,
    // the query worked but matched no records - run config not found
    err = "GetRunDeviceConfig error: config "+name+" for runconfig "+runconfig_name+" version "+std::to_string(runconfig_version)+" not found";
    std::cerr<<err<<std::endl;
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
    std::cerr<<err<<std::endl;
    json_data=err;
    return false;
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool Services::GetROOTplot(const std::string& plot_name, std::string& draw_options, std::string& json_data, int& version, const unsigned int timeout){
  
  std::string cmd_string;
  
  if(version<0){
    // https://stackoverflow.com/questions/tagged/greatest-n-per-group for faster
    cmd_string = "SELECT jsonb_build_object('version', version, 'data', data, 'draw_options', draw_options) FROM rootplots WHERE name='"+plot_name+"' ORDER BY version DESC LIMIT 1";
  } else {
    cmd_string = "SELECT jsonb_build_object('version', version, 'data', data, 'draw_options', draw_options) FROM rootplots WHERE name='"+plot_name+"' AND version="+std::to_string(version);
  }
  
  std::string err="";
  std::string response="";
  
  if(!m_backend_client.SendCommand("R_TROOTPLOT", cmd_string, &response, &timeout, &err)){
    std::cerr<<"GetROOTplot error: "<<err<<std::endl;
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
    std::cerr<<err<<std::endl;
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
    cmd_string = "SELECT jsonb_build_object('version', version, 'data', data, 'layout', layout) FROM plotlyplots WHERE name='"+name+"' ORDER BY version DESC LIMIT 1";
  } else {
    cmd_string = "SELECT jsonb_build_object('version', version, 'data', data, 'layout', layout) FROM plotlyplots WHERE name='"+name+"' AND version="+std::to_string(version);
  }
  
  std::string err;
  std::string response;
  if (!m_backend_client.SendCommand("R_PLOTLYPLOT", cmd_string, &response, &timeout, &err)){
    std::cerr << "GetPlotlyPlot error: " << err << std::endl;
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
    std::cerr<<err<<std::endl;
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

bool Services::SendLog(const std::string& message, unsigned int severity, const std::string& device, const uint64_t timestamp){
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = std::string{"{\"topic\":\"LOGGING\""}
                         + ",\"time\":\""+TimeStringFromUnixMs(timestamp)+"\""
                         + ",\"device\":\""+ name +"\""
                         + ",\"severity\":"+std::to_string(severity)
                         + ",\"message\":\"" +  message + "\"}";
  
  if(cmd_string.length()>MAX_UDP_PACKET_SIZE){
    std::cerr<<"Logging message is too long! Maximum length may be MAX_UDP_PACKET_SIZE bytes"<<std::endl;
    return false;
  }
  
  std::string err="";
  
  if(!m_backend_client.SendMulticast(MulticastType::Log,cmd_string, &err)){
    std::cerr<<"SendLog error: "<<err<<std::endl;
    return false;
  }
  
  return true;
  
}


bool Services::SendMonitoringData(const std::string& json_data, const std::string& subject, const std::string& device, const uint64_t timestamp){
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = std::string{"{\"topic\":\"MONITORING\""}
                         + ", \"time\":\""+TimeStringFromUnixMs(timestamp)+"\""
                         + ", \"device\":\""+ name +"\""
                         + ", \"subject\":\""+ subject +"\""
                         + ", \"data\":"+ json_data +" }";
  
  if(cmd_string.length()>MAX_UDP_PACKET_SIZE){
    std::cerr<<"Monitoring message is too long! Maximum length may be MAX_UDP_PACKET_SIZE bytes"<<std::endl;
    return false;
  }
  
  std::string err="";
  
  if(!m_backend_client.SendMulticast(MulticastType::Monitoring,cmd_string, &err)){
    std::cerr<<"SendMonitoringData error: "<<err<<std::endl;
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
    std::cerr<<"ROOT plot json is too long! Maximum length may be MAX_UDP_PACKET_SIZE bytes"<<std::endl;
    return false;
  }
  
  std::string err="";
  
  if(!m_backend_client.SendMulticast(MulticastType::Log,cmd_string, &err)){
    std::cerr<<"SendROOTplot error: "<<err<<std::endl;
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
  struct tm* timeptr = gmtime(&timestamp_sec);
  if(timeptr==0){
    //Log("gmtime error converting unix time '"+std::to_string(timestamp)+"' to time struct",v_error);
    return "now()";
  }
  char timestring[24];
  int nchars = strftime(&timestring[0], 20, "%F %T", timeptr);
  if(nchars==0){
    //Log("strftime error converting time struct '"+std::to_string(timestamp)+"' to string",v_error);
    return "now()";
  }
  // add the milliseconds
  nchars = snprintf(&timestring[19], 5, ".%03d", timestamp_ms);
  if(nchars!=4){
    //Log("snprintf error converting '"+std::to_string(timestamp_ms)+"' to timestamp milliseconds",v_error);
    //return "now()";  // just omit the milliseconds? or fall back to 'now'?
  }
  
  return std::string{timestring};
  
}
