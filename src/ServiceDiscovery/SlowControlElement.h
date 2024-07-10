#ifndef SLOW_CONTROL_ELEMENT
#define SLOW_CONTROL_ELEMENT

#include <string>
#include <Store.h>
#include <sstream>
#include <mutex>
#include <functional>

namespace ToolFramework{

  //  typedef std::string (*SCFunction)(const char*);
  typedef std::function<std::string(const char*)> SCFunction;
  
  enum SlowControlElementType { BUTTON, VARIABLE, OPTIONS, COMMAND, INFO };
  
  //need to add a new element type for info box that can be used for status
  
  class SlowControlElement{
    
  public:
    
    SlowControlElement(std::string name, SlowControlElementType type, SCFunction change_function = 0, SCFunction read_function = 0);
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
    bool SetDefault(std::string value);
    bool SetValue(std::string value);
    bool GetValue(std::string &value); 

    
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
	tmp<<value;
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
      options.Set("value", value);
      mtx.unlock();
      return true;
    }
    
    
    template<typename T> T GetValue(){
      T tmp;
      mtx.lock();
      options.Get("value", tmp);
      mtx.unlock();
      return tmp;
      
    }
    
    template<typename T> bool GetValue(T &value){
      mtx.lock();
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
    
    std::mutex mtx;
    
  };
  
}

#endif
