#ifndef SERVICES_H
#define SERVICES_H

#include <ServiceDiscovery.h>
#include <zmq.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <functional>
#include <SlowControlCollection.h>
//#include <boost/uuid/uuid.hpp>            // uuid class
//#include <boost/uuid/uuid_generators.hpp> // generators
//#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
//#include <boost/date_time/posix_time/posix_time.hpp>
//#include <boost/progress.hpp>
#include <ServicesBackend.h>
#include <Utilities.h>

#define SERVICES_DEFAULT_TIMEOUT 1800

namespace ToolFramework {
  
  enum class LogLevel { Error=0, Warning=1, Message=2, Debug=3, Debug1=4, Debug2=5, Debug3=6 };
  
  struct LogMsg {
    LogMsg(const std::string& i_message, LogLevel i_severity=LogLevel::Message, const std::string& i_device="", const uint64_t i_timestamp=0) : message{i_message}, severity{i_severity}, device{i_device}, timestamp{i_timestamp} {};
    std::string message;
    LogLevel severity;
    std::string device;
    uint64_t timestamp;
    uint32_t repeats;
  };
  
  struct MonitoringMsg {
    MonitoringMsg(const std::string& i_json_data, const std::string& i_subject, const std::string& i_device="", uint64_t i_timestamp=0) : json_data{i_json_data}, subject{i_subject}, device{i_device}, timestamp{i_timestamp} {};
    std::string json_data;
    std::string subject;
    std::string device;
    uint64_t timestamp;
  };
  
  class Services;
  
  struct BufferThreadArgs : Thread_args {
    Services* services;
    std::vector<LogMsg>* logging_buf;
    std::unordered_map<std::string, MonitoringMsg>* monitoring_buf;
    std::mutex* logging_buf_mtx;
    std::mutex* monitoring_buf_mtx;
    std::chrono::milliseconds multicast_send_period_ms;
    std::chrono::steady_clock::time_point last_send;
    std::string local_merge_buf;
  };
  
  class Services{
    
    
  public:
    
    Services();
    ~Services();
    bool Init(Store &m_variables, zmq::context_t* context_in, SlowControlCollection* sc_vars_in, bool new_service=false);
    bool Ready(const unsigned int timeout=10000); // default service discovery broadcast period is 5s, middleman also checks intermittently, compound total time should be <10s...
    
    bool SQLQuery(const std::string& query, std::vector<std::string>& responses, const unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    bool SQLQuery(const std::string& query, std::string& response, const unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    bool SQLQuery(const std::string& query, const unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    
    // public interface methods - push into local buffer
    bool SendLog(const std::string& message, LogLevel severity=LogLevel::Message, const std::string& device="", const uint64_t timestamp=0);
    bool SendMonitoringData(const std::string& json_data, const std::string& subject, const std::string& device="", uint64_t timestamp=0);
    
    bool SendAlarm(const std::string& message, bool critical=false, const std::string& device="", const uint64_t timestamp=0, const unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    bool SendCalibrationData(const std::string& json_data, const std::string& description, const std::string& device="", uint64_t timestamp=0, int* version=nullptr, const unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    bool GetCalibrationData(std::string& json_data, int& version, const std::string& device="", const unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    bool GetCalibrationData(std::string& json_data, int&& version=-1, const std::string& device="", const unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    bool SendDeviceConfig(const std::string& json_data, const std::string& author, const std::string& description, const std::string& device="", uint64_t timestamp=0, int* version=nullptr, const unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    bool SendBaseConfig(const std::string& json_data, const std::string& name, const std::string& author, const std::string& description, uint64_t timestamp=0, int* version=nullptr, const unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    bool SendRunModeConfig(const std::string& json_data, const std::string& name, const std::string& author, const std::string& description, uint64_t timestamp=0, int* version=nullptr, const unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    bool GetDeviceConfig(std::string& json_data, const int version=-1, const std::string& device="", const unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    bool GetRunConfig(std::string& json_data, const int base_config_id, const int runmode_config_id, const unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    bool GetRunModeConfig(std::string& json_data, const std::string& runmode_name, const int version=-1, const unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    bool GetRunDeviceConfig(std::string& json_data, const int base_config_id, const int runmode_config_id, const std::string& device="", int* version=nullptr, const unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    bool GetCachedDeviceConfig(std::string& json_data, const int base_config_id, const int runmode_config_id, const std::string& device="", int* version=nullptr, unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    bool SendROOTplot(const std::string& plot_name, const std::string& draw_options, const std::string& json_data, int* version=nullptr, const uint64_t timestamp=0, const unsigned int lifetime=5, const unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    bool SendROOTplotMulticast(const std::string& plot_name, const std::string& draw_options, const std::string& json_data, const unsigned int lifetime=5, const uint64_t timestamp=0);
    bool GetROOTplot(const std::string& plot_name, std::string& draw_option, std::string& json_data, int& version, const unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    bool GetROOTplot(const std::string& plot_name, std::string& draw_option, std::string& json_data, int&& version=-1, const unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    bool SendPlotlyPlot(const std::string& name, const std::string& json_trace, const std::string& json_layout="{}", int* version=nullptr, uint64_t timestamp=0, const unsigned int lifetime=5, unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    bool SendPlotlyPlot(const std::string& name, const std::vector<std::string>& json_traces, const std::string& json_layout="{}", int* version=nullptr, uint64_t timestamp=0, const unsigned int lifetime=5, unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    bool GetPlotlyPlot(const std::string& name, std::string& json_trace, std::string& json_layout, int& version, unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    bool GetPlotlyPlot(const std::string& name, std::string& json_trace, std::string& json_layout, int&& version=-1, unsigned int timeout=SERVICES_DEFAULT_TIMEOUT);
    static std::string TimeStringFromUnixMs(const uint64_t time);
    
    SlowControlCollection* GetSlowControlCollection();
    SlowControlElement* GetSlowControlVariable(std::string key);
    bool AddSlowControlVariable(std::string name, SlowControlElementType type, std::function<std::string(const char*)> change_function=nullptr, std::function<std::string(const char*)> read_function=nullptr);
    bool RemoveSlowControlVariable(std::string name);
    void ClearSlowControlVariables();
    
    bool AlertSubscribe(std::string alert, std::function<void(const char*, const char*)> function);
    bool AlertSend(std::string alert, std::string payload);
    
    std::string PrintSlowControlVariables();
    std::string GetDeviceName();
    void SetVerbose(bool in);
    
    template<typename T> T GetSlowControlValue(std::string name){
      return (*sc_vars)[name]->GetValue<T>();
    }
    
    SlowControlCollection* sc_vars;
    
  private:

    std::string LoadConfig1(const char* sc_name);
    void LoadConfig2(const char* alert, const char* payload);
    // private methods for sending from buffer
    bool SendLog(std::string& msg);
    bool SendMonitoringData(std::string& msg);
    static void BufferThread(Thread_args* args);
    
    std::string m_name;
    bool m_verbose;
    zmq::context_t* m_context;
    ServicesBackend m_backend_client;
    std::string m_local_config;
    uint64_t m_base_config_id;
    uint64_t m_run_mode_config_id;

    Utilities m_utils;
    BufferThreadArgs thread_args;
    
    std::vector<LogMsg> logging_buf;
    std::unordered_map<std::string, MonitoringMsg> monitoring_buf;
    std::mutex logging_buf_mtx;
    std::mutex monitoring_buf_mtx;
    uint32_t mon_merge_period_ms;
    uint32_t multicast_send_period_ms;
    
  };
  
}

#endif
