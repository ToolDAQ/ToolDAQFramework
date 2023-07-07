#ifndef SLOW_CONTROL_ELEMENT
#define SLOW_CONTROL_ELEMENT

#include <string>
#include <Store.h>
#include <sstream>
#include <mutex>
#include <functional>

enum SlowControlElementType { BUTTON, VARIABLE, OPTIONS, COMMAND, INFO };

//need to add a new element type for info box that can be used for status

class SlowControlElement{

 public:
 
  SlowControlElement(std::string name, SlowControlElementType type,  std::function<std::string()> function=nullptr);
  std::string GetName();
  bool IsName(std::string name);
  std::string Print();
  std::function<std::string()> GetFunction();
  SlowControlElementType GetType();
  
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
      std::stringstream tmp;
      tmp<<num_options;
      options.Set(tmp.str(), value);
      mtx.unlock();
      return true;
    }
    else return false;
  }

  bool AddCommand(std::string value){
    if(m_type == SlowControlElementType(COMMAND)){
      mtx.lock();
      num_options++;
      std::stringstream tmp;
      tmp<<num_options;
      options.Set(tmp.str(), value);
      mtx.unlock();
      return true;
    }
    else return false;
  }

  template<typename T> bool SetValue(T value){
    mtx.lock();
    options.Set("value", value);
    mtx.unlock();
    return true;
  }

  bool SetValue(const char value[]){
    std::string tmp_value=value;
    return SetValue(tmp_value);
  }

  bool SetValue(std::string value){
    mtx.lock();
    if(m_type == SlowControlElementType(INFO)){ //sanitising for web printout
	for(unsigned int i=0; i<value.length(); i++){
	  if(value.at(i)==',') value.at(i)='.';
	  else if (value.at(i)=='{') value.at(i)='[';
	  else if (value.at(i)=='}') value.at(i)=']';
	  else if (value.at(i)=='"') value.at(i)='\'';
	}
      }
    *(options["value"])= value;
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

  bool GetValue(std::string &value){
    mtx.lock();
    if(!options.Has("value")) return false;
    value=*(options["value"]);
    mtx.unlock();
    return true;
  } 

 private:

  std::string m_name;
  SlowControlElementType m_type;
  Store options;
  unsigned int num_options;
  std::function<std::string()> m_function;

  std::mutex mtx;
 
};

#endif
