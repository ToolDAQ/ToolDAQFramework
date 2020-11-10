#include "Store.h"


Store::Store(){}


void Store::Initialise(std::string filename){
  
  std::ifstream file(filename.c_str());
  std::string line;
  
  if(file.is_open()){
    
    while (getline(file,line)){
      if (line.size()>0){
	if (line.at(0)=='#')continue;
	std::string key;
	std::string value;
	std::stringstream stream(line);
	if(stream>>key>>value) m_variables[key]=value;
	
      }
      
    }
  }
  else std::cout<<"\033[38;5;196m WARNING!!!: Config file "<<filename<<" does not exist no config loaded \033[0m"<<std::endl;
  
  file.close();
  
}

void Store::Print(){
  
  for (std::map<std::string,std::string>::iterator it=m_variables.begin(); it!=m_variables.end(); ++it){
    
    std::cout<< it->first << " => " << it->second <<std::endl;
    
  }
  
}


void Store::Delete(){

  m_variables.clear();


}


void Store::JsonParser(std::string input){ 

  int type=0;
  std::string key;
  std::string value;

  for(std::string::size_type i = 0; i < input.size(); ++i) {
    
     if(input[i]=='\"')type++; 
      else if(type==1)key+=input[i];
      else if(type==3)value+=input[i];
      else if(type==4){
	type=0;
	m_variables[key]=value;
	key="";
	value="";
      }
     
      
      /*
    if(input[i]!=',' &&  input[i]!='{' && input[i]!='}' && input[i]!='\"' && input[i]!=':' && input[i]!=',')pair<<input[i];
    else if(input[i]==':')pair<<" ";
    else if(input[i]==',') {
      std::cout<<" i = "<<i<<" pair = "<<pair<<std::endl;
    
      pair>>key>>value;
      */  
      //pair.clear();

      //}
  }
  
}

bool Store::Has(std::string key){

  return (m_variables.count(key)!=0);

}
