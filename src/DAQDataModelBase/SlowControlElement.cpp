#include <SlowControlElement.h>

using namespace ToolFramework;

SlowControlElement::SlowControlElement(std::string name, SlowControlElementType type, SCFunction change_function, SCFunction read_function){
				       //std::function<std::string(const char*)> function){
  
  m_name=name;
  m_type=type;
  num_options=0;
  m_change_function=change_function;
  m_read_function=read_function;
  
}


std::string SlowControlElement::GetName(){
  
  return m_name;
  
}

bool SlowControlElement::IsName(std::string name){
  
  return (m_name == name);
  
}

std::string SlowControlElement::Print(){
  
  std::string out="{\"name\":\""+m_name+"\",\"type\":\"";
  if(m_type==SlowControlElementType(VARIABLE)) out+="VARIABLE\"";
  else if (m_type==SlowControlElementType(BUTTON)) out+="BUTTON\"";
  else if (m_type==SlowControlElementType(OPTIONS)) out+="OPTIONS\"";
  else if (m_type==SlowControlElementType(COMMAND)) out+="COMMAND\"";
  else if (m_type==SlowControlElementType(INFO)) out+="INFO\"";
  
  mtx.lock();


  for(std::map<std::string,std::string>::iterator it=options.begin(); it!=options.end(); it++){

    out+=",\""+it->first+"\":"+it->second;
  }
  out+="}";
  mtx.unlock();
  return out;
}

/*std::string SlowControlElement::Print(){
  
  std::string out="";
  mtx.lock();
  if(m_type==SlowControlElementType(VARIABLE) || m_type==SlowControlElementType(OPTIONS)) out+="[";
  
  out+=m_name;
  
  if(m_type==SlowControlElementType(VARIABLE)){
    std::string min=options.Get<std::string>("min");
    std::string max=options.Get<std::string>("max");
    std::string step=options.Get<std::string>("step");
    std::string value=options.Get<std::string>("value");
    
    
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
    std::string value=options.Get<std::string>("value");
    
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
    std::string value=options.Get<std::string>("value");
    out+="{" + value + "}";
  }
  
  mtx.unlock();
  return out;
}
*/

bool SlowControlElement::JsonParser(std::string json){

  mtx.lock();
  Store tmp;
  tmp.JsonParser(json);
  //m_name=tmp.Get<std::string>("name");
  std::string type=tmp.Get<std::string>("type");
  if(type=="BUTTON" || type=="button") m_type=SlowControlElementType::BUTTON;
  else if(type=="VARIABLE" || type=="variable"){
    m_type=SlowControlElementType::VARIABLE;
    SetMin(tmp.Get<double>("min"));
    SetMax(tmp.Get<double>("max"));
    SetStep(tmp.Get<double>("step"));
    SetValue(tmp.Get<double>("value"));
  }
  else if(type=="OPTIONS" || type=="options"){
    m_type=SlowControlElementType::OPTIONS;
    SetValue(tmp.Get<std::string>("value"));
    std::string options="";
    tmp.Get("options",options);
    unsigned int first=1;
    for(unsigned int i=1; i<options.length(); i++){
      if(options[i]==']' || options[i]==','){ //will go wrong if an option has a comma
	AddOption(options.substr(first, i-first));
	first=i+1;
      }
    }
  }
  else if(type=="COMMAND" || type=="command"){
    m_type=SlowControlElementType::COMMAND;
    std::string commands="";
    tmp.Get("commands",commands);
    SetDefault(tmp.Get<std::string>("default"));
    unsigned int first=1;
    for(unsigned int i=1; i<commands.length(); i++){
      if(commands[i]==']' || commands[i]==','){ //will go wrong if an option has a comma
	AddCommand(commands.substr(first, i-first));
	first=i+1;
      }
    }
  }
  else if(type=="INFO" || type=="info"){
    m_type=SlowControlElementType::INFO;
    SetValue(tmp.Get<std::string>("value"));
  }
  else{
    mtx.unlock();
    return false;
  }
  
  mtx.unlock();
  return true;
}

SCFunction SlowControlElement::GetChangeFunction(){
  
  return m_change_function;
  
}

SCFunction SlowControlElement::GetReadFunction(){
  
  return m_read_function;
  
}

SlowControlElementType SlowControlElement::GetType(){
  
  return m_type;
  
}

bool SlowControlElement::AddCommand(std::string value){
  if(m_type == SlowControlElementType(COMMAND)){
    mtx.lock();
    num_options++;
    std::string current;
    std::stringstream tmp;
    tmp<<value;
    if(!options.Get("commands",current)) current="["+tmp.str()+"]";
    else {
      current=current.substr(0,current.length()-1);
      current+="," + tmp.str() + "]";
    }
    options.Set("commands", current);
    options.Destring("commands");
    mtx.unlock();
    return true;
  }
  else return false;
}

bool SlowControlElement::SetDefault(std::string value){
  
  if(m_type != SlowControlElementType(COMMAND)) return false;
  mtx.lock();
  options.Set("default",value);
  mtx.unlock();
  return true;
}


bool SlowControlElement::SetValue(const char value[]){
  bool ret=false;
  std::string tmp_value=value;
  mtx.lock();
  ret=SetValue(tmp_value);
  mtx.unlock();
  return ret;
}


bool SlowControlElement::SetValue(std::string value){
  mtx.lock();
  if(m_type == SlowControlElementType(VARIABLE)){
    std::stringstream tmp(value);
    double val=0;
    tmp>>val;
    mtx.unlock();
    return SetValue(val);
  }
  else if(m_type == SlowControlElementType(INFO)){ //sanitising for web printout
    for(unsigned int i=0; i<value.length(); i++){
      if(value.at(i)==',') value.at(i)='.';
      else if (value.at(i)=='{') value.at(i)='[';
      else if (value.at(i)=='}') value.at(i)=']';
      else if (value.at(i)=='"') value.at(i)='\'';
    }
  }
  options.Set("value", value);
  mtx.unlock();
  return true;
}


bool SlowControlElement::GetValue(std::string &value){
  mtx.lock();
  if(!options.Has("value")){
    mtx.unlock();
    return false;
  }
  options.Get("value", value);
  mtx.unlock();
  return true;
} 
