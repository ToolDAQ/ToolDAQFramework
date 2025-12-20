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

bool Services::SendAlarm(const std::string& message, unsigned int level, const std::string& device, const uint64_t timestamp, const unsigned int timeout){
  
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
  // SendAlarm returns nothing
  
  // also record it to the logging socket
  cmd_string = std::string{"{ \"topic\":\"LOGGING\""}
             + ",\"time\":"+std::to_string(timestamp)
             + ",\"device\":\""+name+"\""
             + ",\"severity\":0"
             + ",\"message\":\"" + message + "\"}";
  
  ok = ok && m_backend_client.SendMulticast(MulticastType::Log,cmd_string, &err);
  
  if(!ok){
    std::clog<<"SendAlarm (log) error: "<<err<<std::endl;
  }
  
  return ok;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool Services::SendCalibrationData(const std::string& json_data, const std::string& description, const std::string& name, const uint64_t timestamp, int* version, const unsigned int timeout){
  
  const std::string& c_name = (name=="") ? m_name : name;
  
  std::string cmd_string = "{ \"time\":"+std::to_string(timestamp)
                         + ", \"name\":\""+ c_name +"\""
                         + ", \"description\":\""+ description+"\""
                         + ", \"data\":\""+ json_data +"\" }";
  
  std::string err="";
  std::string response="";
  
  if(!m_backend_client.SendCommand("W_CALIBRATION", cmd_string, &response, &timeout, &err)){
    std::clog<<"SendCalibrationData error: "<<err<<std::endl;
    return false;
  }
  
  if(response.empty()){
    std::clog<<"SendCalibrationData error: empty response"<<std::endl;
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

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool Services::SendDeviceConfig(const std::string& json_data, const std::string& author, const std::string& description, const std::string& device, const uint64_t timestamp, int* version, const unsigned int timeout){
  
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
  
  if(response.empty()){
    std::clog<<"SendDeviceConfig error: empty response"<<std::endl;
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

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool Services::SendRunConfig(const std::string& json_data, const std::string& name, const std::string& author, const std::string& description, const uint64_t timestamp, int* version, const unsigned int timeout){
  
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
  
  if(response.empty()){
    std::clog<<"SendRunConfig error: empty response"<<std::endl;
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

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool Services::SendROOTplotZmq(const std::string& plot_name, const std::string& draw_options, const std::string& json_data, const unsigned int keep_until, int* version, const uint64_t timestamp, const unsigned int timeout){
  
  std::string cmd_string = "{ \"time\":"+std::to_string(timestamp)
                         + ", \"name\":\""+ plot_name +"\""
                         + ", \"draw_options\":\""+ draw_options +"\""
                         + ", \"data\":\""+ json_data+"\""
                         + ", \"keep_until\":"+std::to_string(keep_until)+" }"; // FIXME modify DB as per retention
  
  std::string err="";
  std::string response="";
  
  if(!m_backend_client.SendCommand("W_TROOTPLOT", cmd_string, &response, &timeout, &err)){
    std::clog<<"SendROOTplot error: "<<err<<std::endl;
    return false;
  }
  
  if(response.empty()){
    std::clog<<"SendROOTplot error: empty response"<<std::endl;
    return false;
  }
  
  // response is json with the version number of the created plot entry
  // e.g. '{"version":3}'. check this is what we got, as validation.
  Store tmp;
  tmp.JsonParser(response);
  int tmp_version;
  bool ok = tmp.Get("version",tmp_version);
  if(ok && version){
    *version = tmp_version;
  }
  if(!ok){
    std::clog<<"SendROOTplot error: invalid response: '"<<response<<"'"<<std::endl;
    return false;
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
    const unsigned int timeout
) {
  return SendPlotlyPlot(
      name,
      std::vector<std::string> { trace },
      layout,
      version,
      timestamp,
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
    const unsigned int              timeout
) {
  std::stringstream ss;
  ss << "{\"name\":\"" << name << "\",\"layout\":" << layout;
  if (timestamp) ss << ",\"time\":" << timestamp;
  ss << ",\"data\":[";
  bool first = true;
  for (auto& trace : traces) {
    if (first)
      first = false;
    else
      ss << ',';
    ss << trace;
  };
  ss << "]}";
  
  std::string response="";
  
  std::string err;
  if (!m_backend_client.SendCommand("W_PLOTLYPLOT", ss.str(), &response, &timeout, &err)){
    std::clog << "SendPlotlyPlot error: " << err << std::endl;
    return false;
  };
  
  if(response.empty()){
    std::clog<<"SendPlotlyPlot error: empty response"<<std::endl;
    return false;
  }
  
  // response is json with the version number of the created plot entry
  // e.g. '{"version":3}'. check this is what we got, as validation.
  Store tmp;
  tmp.JsonParser(response);
  int tmp_version;
  bool ok = tmp.Get("version",tmp_version);
  if(ok && version){
    *version = tmp_version;
  }
  if(!ok){
    std::clog<<"SendPlotlyPlot error: invalid response: '"<<response<<"'"<<std::endl;
    return false;
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
    std::clog<<"SQLQuery error: "<<err<<std::endl;
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
      std::clog<<"Warning: SQLQuery returned multiple rows, only first returned"<<std::endl;
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
    cmd_string = "SELECT row_to_json(data, version) FROM calibration WHERE name='"+name+"' ORDER BY version DESC LIMIT 1";
  } else {
    cmd_string = "SELECT row_to_json(data, version) FROM calibration WHERE device='"+name+"' AND version="+std::to_string(version);
  }
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_CALIBRATION", cmd_string, &json_data, &timeout, &err)){
    std::clog<<"GetCalibrationData error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(json_data.empty()){
    // if we got an empty response but the command succeeded,
    // the query worked but matched no records - run config not found
    err = "GetCalibrationData error: data for device "+name+" version "+std::to_string(version)+" not found";
    std::clog<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  Store tmp;
  tmp.JsonParser(json_data);
  int tmp_version;
  bool ok = tmp.Get("version",tmp_version);
  if(ok && version<0){
    version = tmp_version;
  }
  ok = ok && tmp.Get("data",json_data);
  
  if(!ok){
    err = "GetCalibrationData error: invalid response: '"+response+"'";
    std::clog<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool Services::GetDeviceConfig(std::string& json_data, int& version, const std::string& device, const unsigned int timeout){

  json_data="";
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = "SELECT row_to_json(data) FROM device_config WHERE name='"+name+"' AND version="+std::to_string(version);
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_DEVCONFIG", cmd_string, &json_data, &timeout, &err)){
    std::clog<<"GetDeviceConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(json_data.empty()){
    // if we got an empty response but the command succeeded,
    // the query worked but matched no records - run config not found
    err = "GetDeviceConfig error: config for device "+name+" version "+std::to_string(version)+" not found";
    std::clog<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  // response format '{"data":"<contents>"}' - strip out contents
  Store tmp;
  tmp.JsonParser(json_data);
  bool ok = tmp.Get("data", json_data);
  if(!ok){
    err = "GetDeviceConfig error: invalid response: '"+json_data+"'";
    std::clog<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

// get a run configuration via configuration ID
bool Services::GetRunConfig(std::string& json_data, const int config_id, const unsigned int timeout){

  json_data="";
  
  std::string cmd_string = "SELECT row_to_json(data) FROM run_config WHERE config_id="+std::to_string(config_id);
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_RUNCONFIG", cmd_string, &json_data, &timeout, &err)){
    std::clog<<"GetRunConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(json_data.empty()){
    // if we got an empty response but the command succeeded,
    // the query worked but matched no records - run config not found
    err = "GetRunConfig error: config_id "+std::to_string(config_id)+" not found";
    std::clog<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  // response format '{"data":<contents>}' - strip out contents
  Store tmp;
  tmp.JsonParser(json_data);
  bool ok = tmp.Get("data", json_data);
  if(!ok){
    err="GetRunConfig error: invalid response: '"+json_data+"'";
    std::clog<<err<<std::endl;
    json_data=err;
    return false;
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

// get a run configuration by name and version (e.g. name: AmBe, version: 3)
bool Services::GetRunConfig(std::string& json_data, const std::string& name, const int version, const unsigned int timeout){

  json_data="";
  
  std::string cmd_string = "SELECT row_to_json(data) FROM run_config WHERE name='"+name+"' AND version="+std::to_string(version);
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_RUNCONFIG", cmd_string, &json_data, &timeout, &err)){
    std::clog<<"GetRunConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(json_data.empty()){
    // if we got an empty response but the command succeeded,
    // the query worked but matched no records - run config not found
    err = "GetRunConfig error: config "+name+" version "+std::to_string(version)+" not found";
    std::clog<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  // response format '{"data":"<contents>"}' - strip out contents
  Store tmp;
  tmp.JsonParser(json_data);
  bool ok = tmp.Get("data", json_data);
  if(!ok){
    err="GetRunConfig error: invalid response: '"+json_data+"'";
    std::clog<<err<<std::endl;
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
  
  std::string cmd_string = "SELECT row_to_json(data, version) FROM device_config WHERE name='"+name+"' AND version=(SELECT data->>'"+name+"' FROM run_config WHERE config_id="+std::to_string(runconfig_id)+")";
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_DEVCONFIG", cmd_string, &json_data, &timeout, &err)){
    std::clog<<"GetRunDeviceConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(json_data.empty()){
    // if we got an empty response but the command succeeded,
    // the query worked but matched no records - run config not found
    err = "GetRunDeviceConfig error: config "+name+" for runconfig "+std::to_string(runconfig_id)+" not found"
    std::clog<<err<<std::endl;
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
    std::clog<<err<<std::endl;
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
  
    std::string cmd_string = "SELECT row_to_json(data, version) FROM device_config WHERE name='"+name+"' AND version=(SELECT data->>'"+name+"' FROM run_config WHERE name='"+runconfig_name+"' AND version="+std::to_string(runconfig_version)+")";
  
  std::string err="";
  
  if(!m_backend_client.SendCommand("R_DEVCONFIG", cmd_string, &json_data, &timeout, &err)){
    std::clog<<"GetRunDeviceConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  if(json_data.empty()){
    // if we got an empty response but the command succeeded,
    // the query worked but matched no records - run config not found
    err = "GetRunDeviceConfig error: config "+name+" for runconfig "+runconfig_name+" version "+std::to_string(runconfig_version)+" not found";
    std::clog<<err<<std::endl;
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
    std::clog<<err<<std::endl;
    json_data=err;
    return false;
  }
  
  return true;
  
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool Services::GetROOTplot(const std::string& plot_name, int& version, std::string& draw_options, std::string& json_data, std::string* timestamp, const unsigned int timeout){
  
  std::string cmd_string;
  
  if(version<0){
    cmd_string = "SELECT row_to_json(version, time, data, draw_options), FROM rootplots WHERE name='"+plot_name+"' AND version="+std::to_string(version);
  } else {
    // https://stackoverflow.com/questions/tagged/greatest-n-per-group for faster
    cmd_string = "SELECT row_to_json(version, time, data, draw_options) FROM rootplots WHERE name='"+plot_name+"' ORDER BY version DESC LIMIT 1";
  }
  
  std::string err="";
  std::string response="";
  
  if(!m_backend_client.SendCommand("R_TROOTPLOT", cmd_string, &response, &timeout, &err)){
    std::clog<<"GetROOTplot error: "<<err<<std::endl;
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
  if(timestamp && ok) ok = ok && plot.Get("time", *timestamp);
  
  if(!ok){
    err="GetROOTplot error: invalid response: '"+response+"'";
    std::clog<<err<<std::endl;
    json_data=err;
    return false;
  }
  
  /*
  std::clog<<"timestamp: "<<timestamp<<"\n"
           <<"draw opts: "<<draw_options<<"\n"
           <<"json data: '"<<json_data<<"'"<<std::endl;
  */
  
  return true;
  
}

// Get a device configuration from a *run* configuration ID

bool Services::GetPlotlyPlot(
    const std::string& name,
    int&               version,
    std::string&       trace,
    std::string&       layout,
    std::string*       timestamp,
    unsigned int       timeout
) {
  
  std::string cmd_string;
  if(version<0){
    cmd_string = "SELECT row_to_json(version, time, data, layout), FROM plotlyplots WHERE name='"+name+"' AND version="+std::to_string(version);
  } else {
    // https://stackoverflow.com/questions/tagged/greatest-n-per-group for faster
    cmd_string = "SELECT row_to_json(version, time, data, layout) FROM plotlyplots WHERE name='"+name+"' ORDER BY version DESC LIMIT 1";
  }
  
  std::string err;
  std::string response;
  if (!m_backend_client.SendCommand("R_PLOTLYPLOT", cmd_string, &response, &timeout, &err)){
    std::clog << "GetPlotlyPlot error: " << err << std::endl;
    json_data = err;
    return false;
  };
  
  if(response.empty()){
    err = "GetPlotlyPlot error: PlotlyPlot "+name+" version "+std::to_string(version)+" not found";
    json_data = err;
    return false;
  }
  
  Store plot;
  plot.JsonParser(response);
  
  trace   = plot.Get<std::string>("data");
  layout  = plot.Get<std::string>("layout");
  version = plot.Get<int>("version");
  
  bool ok = plot.Get("data", trace);
  ok = ok && plot.Get("layout", layout);
  ok = ok && plot.Get("version", version);
  if(timestamp && ok) ok = ok && plot.Get("time", *timestamp);
  
  if(!ok){
    err="GetPlotlyPlot error: invalid response: '"+response+"'";
    std::clog<<err<<std::endl;
    json_data=err;
    return false;
  }
  
  return true;
}


// ===========================================================================
// Multicast Senders
// -----------------

bool Services::SendLog(const std::string& message, unsigned int severity, const std::string& device, const uint64_t timestamp){
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = std::string{"{ \"topic\":\"LOGGING\""}
                         + ",\"time\":"+std::to_string(timestamp)
                         + ",\"device\":\""+ name +"\""
                         + ",\"severity\":"+std::to_string(severity)
                         + ",\"message\":\"" +  message + "\"}";
  
  if(cmd_string.length()>655355){
    std::clog<<"Logging message is too long! Maximum length may be 655355 bytes"<<std::endl;
    return false;
  }
  
  std::string err="";
  
  if(!m_backend_client.SendMulticast(MulticastType::Log,cmd_string, &err)){
    std::clog<<"SendLog error: "<<err<<std::endl;
    return false;
  }
  
  return true;
  
}


bool Services::SendMonitoringData(const std::string& json_data, const std::string& subject, const std::string& device, const uint64_t timestamp){
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = std::string{"{ \"topic\":\"MONITORING\""}
                         + ", \"time\":"+std::to_string(timestamp)
                         + ", \"device\":\""+ name +"\""
                         + ", \"subject\":\""+ subject +"\""
                         + ", \"data\":\""+ json_data +"\" }";
  
  if(cmd_string.length()>655355){
    std::clog<<"Monitoring message is too long! Maximum length may be 655355 bytes"<<std::endl;
    return false;
  }
  
  std::string err="";
  
  if(!m_backend_client.SendMulticast(MulticastType::Monitoring,cmd_string, &err)){
    std::clog<<"SendMonitoringData error: "<<err<<std::endl;
    return false;
  }
  
  return true;
  
}

// send ROOT plot over multicast
bool Services::SendROOTplotMulticast(const std::string& plot_name, const std::string& draw_options, const std::string& json_data, const uint64_t timestamp){
  
  std::string cmd_string = std::string{"{ \"topic\":\"TROOTPLOT\""}
                         + ", \"time\":"+std::to_string(timestamp)
                         + ", \"name\":\""+ plot_name +"\""
                         + ", \"draw_options\":\""+ draw_options +"\""
                         + ", \"data\":\""+ json_data+"\" }";
  
  if(cmd_string.length()>655355){
    std::clog<<"ROOT plot json is too long! Maximum length may be 655355 bytes"<<std::endl;
    return false;
  }
  
  std::string err="";
  
  if(!m_backend_client.SendMulticast(MulticastType::Log,cmd_string, &err)){
    std::clog<<"SendROOTplot error: "<<err<<std::endl;
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
