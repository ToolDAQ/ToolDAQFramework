#ifndef SERVICES_H
#define SERVICES_H

#include <ServiceDiscovery.h>
#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <functional>
#include <SlowControlCollection.h>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/progress.hpp>
#include <ServicesBackend.h>

namespace ToolFramework {

  struct Plot {
    std::string name;
    std::string title;
    std::string xlabel;
    std::string ylabel;
    std::vector<float> x;
    std::vector<float> y;
    Store info;
  };
  
  class Services{
    
    
  public:
    
    Services();
    ~Services();
    bool Init(Store &m_variables, zmq::context_t* context_in, SlowControlCollection* sc_vars_in, bool new_service=false);
    
    bool SQLQuery(const std::string& database, const std::string& query, std::vector<std::string>* responses=nullptr, const unsigned int timeout=300);
    bool SQLQuery(const std::string& database, const std::string& query, std::string* response=nullptr, const unsigned int timeout=300);
    
    bool SendLog(const std::string& message, unsigned int severity=2, const std::string& device="", const unsigned int timestamp=0);
    bool SendAlarm(const std::string& message, unsigned int level=0, const std::string& device="", const unsigned int timestamp=0, const unsigned int timeout=300);
    bool SendMonitoringData(const std::string& json_data, const std::string& device="", unsigned int timestamp=0);
    bool SendCalibrationData(const std::string& json_data, const std::string& description, const std::string& device="", unsigned int timestamp=0, int* version=nullptr, const unsigned int timeout=300);
    bool GetCalibrationData(std::string& json_data, int version=-1, const std::string& device="", const unsigned int timeout=300);
    bool SendDeviceConfig(const std::string& json_data, const std::string& author, const std::string& description, const std::string& device="", unsigned int timestamp=0, int* version=nullptr, const unsigned int timeout=300);
    bool SendRunConfig(const std::string& json_data, const std::string& name, const std::string& author, const std::string& description, unsigned int timestamp=0, int* version=nullptr, const unsigned int timeout=300);
    bool GetDeviceConfig(std::string& json_data, const int version=-1, const std::string& device="", const unsigned int timeout=300);
    bool GetRunConfig(std::string& json_data, const int config_id=-1, const unsigned int timeout=300);
    bool GetRunConfig(std::string& json_data, const std::string& name, const int version=-1, const unsigned int timeout=300);
    bool GetRunDeviceConfig(std::string& json_data, const int runconfig_id, const std::string& device="", int* version=nullptr, const unsigned int timeout=300);
    bool GetRunDeviceConfig(std::string& json_data, const std::string& runconfig_name, const int runconfig_version, const std::string& device="", int* version=nullptr, const unsigned int timeout=300);
    bool SendROOTplot(const std::string& plot_name, const std::string& draw_options, const std::string& json_data, bool persistent=false, int* version=nullptr, const unsigned int timestamp=0, const unsigned int timeout=300);
    bool SendTemporaryROOTplot(const std::string& plot_name, const std::string& draw_options, const std::string& json_data, int* version=nullptr, const unsigned int timestamp=0);
    bool SendPersistentROOTplot(const std::string& plot_name, const std::string& draw_options, const std::string& json_data, int* version=nullptr, const unsigned int timestamp=0, const unsigned int timeout=300);
    bool GetROOTplot(const std::string& plot_name, int& version, std::string& draw_option, std::string& json_data, std::string* timestamp=nullptr, const unsigned int timeout=300);
    bool SendPlot(Plot& plot, unsigned timeout=300);
    bool GetPlot(const std::string& name, Plot& plot, unsigned timeout=300);
    
    SlowControlCollection* GetSlowControlCollection();
    SlowControlElement* GetSlowControlVariable(std::string key);
    bool AddSlowControlVariable(std::string name, SlowControlElementType type, std::function<std::string(const char*)> change_function=nullptr, std::function<std::string(const char*)> read_function=nullptr);
    bool RemoveSlowControlVariable(std::string name);
    void ClearSlowControlVariables();
    
    bool AlertSubscribe(std::string alert, std::function<void(const char*, const char*)> function);
    bool AlertSend(std::string alert, std::string payload);
    
    std::string PrintSlowControlVariables();
    std::string GetDeviceName();
    
    template<typename T> T GetSlowControlValue(std::string name){
      return (*sc_vars)[name]->GetValue<T>();
    }
    
    SlowControlCollection* sc_vars;
    
  private:
    
    zmq::context_t* m_context;
    ServicesBackend m_backend_client;
    std::string m_dbname;
    std::string m_name;
    
    
  };
  
}

#endif
