#include <SlowControlElement.h>

using namespace ToolFramework;

SlowControlElement::SlowControlElement(std::string name, SlowControlElementType type, std::function<std::string(const char*)> function){
  
  m_name=name;
  m_type=type;
  num_options=0;
  m_function=function;
  
}


std::string SlowControlElement::GetName(){
  
  return m_name;
  
}

bool SlowControlElement::IsName(std::string name){
  
  return (m_name == name);
  
}


std::string SlowControlElement::Print(){
  
  std::string out="";
  mtx.lock();
  if(m_type==SlowControlElementType(VARIABLE) || m_type==SlowControlElementType(OPTIONS)) out+="[";
  
  out+=m_name;
  
  if(m_type==SlowControlElementType(VARIABLE)){
    std::string min=*options["min"];
    std::string max=*options["max"];
    std::string step=*options["step"];
    std::string value=*options["value"];
    
    
    out+=":" + min + ":" + max + ":" + step + ":" + value + "]";
    
  }
  else if(m_type==SlowControlElementType(OPTIONS)){
    
    for(unsigned int i=1; i<num_options+1; i++){
      std::string tmp="";
      std::stringstream key;
      key<<i;
      options.Get(key.str(), tmp);
      out+=";" + tmp;
      
    }
    std::string value=*options["value"];
    
    out+= ";" + value + "]";
  }
  
  else if(m_type==SlowControlElementType(COMMAND)){
    for(unsigned int i=1; i<num_options+1; i++){
      std::string tmp="";
      std::stringstream key;
      key<<i;
      options.Get(key.str(), tmp);
      out+="<" + tmp + ">";
      
    }
  
  }
  
  else if(m_type==SlowControlElementType(INFO)){
    std::string value=*options["value"];
    out+="{" + value + "}";
  }
  
  mtx.unlock();
  return out;
}


std::function<std::string(const char*)> SlowControlElement::GetFunction(){
  
  return m_function;
  
}

SlowControlElementType SlowControlElement::GetType(){
  
  return m_type;
  
}
