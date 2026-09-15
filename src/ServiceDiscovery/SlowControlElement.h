#ifndef SLOW_CONTROL_ELEMENT
#define SLOW_CONTROL_ELEMENT

#include <string>
#include <Store.h>
#include <sstream>
#include <mutex>
#include <functional>

namespace ToolFramework{

  //  typedef std::string (*SCFunction)(const char*);
  typedef std::function<bool(const char*, const char*, std::string&)> SCFunction;
  
  enum SlowControlElementType { BUTTON, VARIABLE, OPTIONS, COMMAND, INFO };
  
  //need to add a new element type for info box that can be used for status
  
  class SlowControlElement{
    
  public:
    
    SlowControlElement(std::string name, SlowControlElementType type, SCFunction change_function = 0, SCFunction read_function = 0, bool lockable=true, bool hidden=false);
		       //std::function<std::string(const char*)> function=nullptr);
    std::string GetName();
    bool IsName(std::string name);
    std::string Print();
    bool JsonParser(std::string json);
    SCFunction GetChangeFunction();
    SCFunction GetReadFunction();
    SlowControlElementType GetType();
    bool AddCommand(std::string value);
    bool SetValue(const char value[]);
    bool SetValue(const char value[], std::string& response);
    bool SetDefault(std::string value);
    bool SetValue(std::string value);
    bool SetValue(std::string value, std::string& response);
    bool GetValue(std::string &value); 
    bool GetValue(std::string &value, std::string& response); 
    bool Lockable();
    bool Hidden();
    
    template<typename T> bool SetMin(T value){ 
      if(m_type == SlowControlElementType(VARIABLE)){
	mtx.lock();
	options.Set("min",value);
	mtx.unlock();
	return true;
      }
      else return false;
    }
    
    template<typename T> bool SetMax(T value){ 
      if(m_type == SlowControlElementType(VARIABLE)){
	mtx.lock();     
	options.Set("max",value);
	mtx.unlock();
	return true;
      }
      else return false;
    }
    
    template<typename T> bool SetStep(T value){ 
      if(m_type == SlowControlElementType(VARIABLE)){
	mtx.lock();      
	options.Set("step",value);
	mtx.unlock(); 
	return true;
      }
      else return false;
    }
    
    template<typename T> bool AddOption(T value){
      if(m_type == SlowControlElementType(OPTIONS)){
	mtx.lock();
	num_options++;
	std::string current;
	std::stringstream tmp;
	tmp<<"\""<<value<<"\"";
	if(!options.Get("options",current)) current="["+tmp.str()+"]";
	else {
	  current=current.substr(0,current.length()-1);
	  current+="," + tmp.str() + "]";
	}
	options.Set("options", current);
	options.Destring("options");
	mtx.unlock();
	return true;
      }
      else return false;
    }
    

    
    template<typename T> bool SetValue(T value){
      std::string tmp ="";
      return SetValue(value, tmp);
    }

    template<typename T> bool SetValue(T value, std::string& response){
      mtx.lock();
      if(m_type == SlowControlElementType(VARIABLE)){
	T min;
	T max;  
	if(options.Get("min",min)){
	  if(value<min) value=min;
	}
	if(options.Get("max",max)){
	  if(value>max) value=max;
	}
      }
      if(m_change_function!=0){
	std::stringstream tmp;
	tmp<<value;
	bool ret = false;
	try{
	  ret = m_change_function(tmp.str().c_str(), m_name.c_str(), response);
	}
	catch(...){
	  ret = false;
	}
	if(!ret){
	  std::cerr<<"failed to call change fucntion "<<m_name<<" : "<<response<<std::endl;
	  mtx.unlock();
	  return false;
	}
      }
      options.Set("value", value);
      mtx.unlock();
      return true;
    }
    
    
    template<typename T> T GetValue(){
      T tmp;
      mtx.lock();
      if(m_read_function!=0){
	bool ret = false;
	std::string response = "";
	try{
	  ret = m_read_function("",m_name.c_str(), response);
	  if(ret) options.Set("value", response);
	}
	catch(...){
	  ret = false;
	}
	if(!ret){
	  std::cerr<<"failed to call read fucntion "<<m_name<<" : "<<response<<std::endl;
	  mtx.unlock();
	  return tmp;
	}
      }
      
      options.Get("value", tmp);
      mtx.unlock();
      return tmp;
      
    }
    
    template<typename T> bool GetValue(T &value){
      std::string tmp ="";                                                                             
      return GetValue(value, tmp);           
    }

    template<typename T> bool GetValue(T &value, std::string& response){
      mtx.lock();  
      if(m_read_function!=0){
	bool ret = false;
	try{
	  ret = m_read_function("", m_name.c_str(), response);
	  if(ret) options.Set("value", response); 
	}
	catch(...){
	  ret= false;
	}
	if(!ret){
	  std::cerr<<"failed to call read fucntion "<<m_name<<" : "<<response<<std::endl;
	  mtx.unlock();
	  return false;
	}
      }
      bool ret=options.Get("value", value);
      mtx.unlock();
      return ret;
    }
    
    
 private:
    
    std::string m_name;
    SlowControlElementType m_type;
    Store options;
    unsigned int num_options;
    SCFunction m_change_function;
    SCFunction m_read_function;
    bool m_lockable;
    bool m_hidden;
    
    std::mutex mtx;
    
  };
  
}

#endif
