#ifndef SLOW_CONTROL_ELEMENT
#define SLOW_CONTROL_ELEMENT

#include <string>
#include <Store.h>
#include <sstream>
#include <mutex>

enum SlowControlElementType { BUTTON, VARIABLE, OPTIONS, COMMAND, INFO };

//need to add a new element type for info box that can be used for status

class SlowControlElement{

 public:
 
  SlowControlElement(std::string name, SlowControlElementType type);
  std::string GetName();
  bool IsName(std::string name);
  std::string Print();
  
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

  template<typename T> T GetValue(){
    
    T tmp;
    mtx.lock();
    options.Get("value", tmp);
    mtx.unlock();
    return tmp;

  }
  
  template<typename T> bool GetValue(T &value){
    mtx.lock();
    options.Get("value", value);
    mtx.unlock();
    return true;
  }
  
 private:

  std::string m_name;
  SlowControlElementType m_type;
  Store options;
  unsigned int num_options;

  std::mutex mtx;
 
};

#endif
