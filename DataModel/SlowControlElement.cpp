#include <SlowControlElement.h>

SlowControlElement::SlowControlElement(std::string name, SlowControlElementType type){

  m_name=name;
  m_type=type;
  num_options=0;
  
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

    for(int i=1; i<num_options+1; i++){
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
    for(int i=1; i<num_options+1; i++){
      std::string tmp="";
      std::stringstream key;
      key<<i;
      options.Get(key.str(), tmp);
      out+="<" + tmp + ">";
   
    }


  }
  mtx.unlock();
  return out;
}
