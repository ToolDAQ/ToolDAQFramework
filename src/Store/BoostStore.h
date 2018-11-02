///////////////////////////////////////////////////////////////////////////////////////////////
/////////////// Universal binary store created by Dr. Benjamin Rirchards (b.richards@qmul.ac.uk)
//////////////////////////////////////////////////////////////////////////////////////////////


#ifndef BOOSTSTORE_H
#define BOOSTSTORE_H

#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <typeinfo>
#include <SerialisableObject.h>
#include <PointerWrapper.h>
#include <stdio.h>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <ctime>
#include <stdlib.h> 

class BoostStore{
  
 public:
  
 BoostStore(bool typechecking=true, int format=0): m_typechecking(typechecking),m_format(format) {findheader();oarch=0;arch=0;totalentries=0;
    if(m_format==2){
      Header = new BoostStore(typechecking,0);
    }
    
  } // format 0=binary, 1=text, 2=multievent.
 BoostStore(std::map<std::string,std::string> invariables):m_variables(invariables){findheader();oarch=0;arch=0;}
 BoostStore(std::map<std::string,std::string> invariables, std::map<std::string,std::string> ininfo):m_variables(invariables), m_type_info(ininfo){findheader();oarch=0;arch=0;}
  bool Initialise(std::string filename, int type=0); //type 0=boost archive, config file 
  void JsonParser(std::string input); 
  void Print(bool values=true);
  void Delete();
  void Remove(std::string key);
  void Save(std::string fimename="Output");
  std::string Type(std::string key);
  bool GetEntry(unsigned long entry);
  bool Close();
  bool Has(std::string key);
  ~BoostStore();
  BoostStore *Header;
  
  
  template<typename T> bool Get(std::string name,T &out){
    
    if(m_variables.count(name)>0){

      if(m_type_info[name]==typeid(out).name() || !m_typechecking){

	std::stringstream stream(m_variables[name]);
	stream.str(m_archiveheader+stream.str());
	if(m_format==0||m_format==2){
	  boost::archive::binary_iarchive ia(stream);
	  ia & out;

	}
	else{
	  boost::archive::text_iarchive ia(stream);
	  ia & out;
	}
	
	return true;
      }
      else return false;
    }
    
    else return false;
    
  }


  template<typename T> bool Get(std::string name,T* &out){  

    if(m_variables.count(name)>0 || m_ptrs.count(name)>0){
      if((m_type_info[name]==typeid(T).name() || !m_typechecking ) || (m_ptrs.count(name)>0 && m_variables.count(name)==0)){
	if(m_ptrs.count(name)==0){
	  
	  T* tmp=new T;
	  m_ptrs[name]=new PointerWrapper<T>(tmp);
	  std::stringstream stream(m_variables[name]);
	  stream.str(m_archiveheader+stream.str());
	  
	  
	  if(m_format==0 || m_format==2){
	    
	    boost::archive::binary_iarchive ia(stream);
	    ia & *tmp;
	    
	  }
	  else{
	    boost::archive::text_iarchive ia(stream);
	    ia & *tmp;
	  }
	}
	
	
	PointerWrapper<T>* tmp=static_cast<PointerWrapper<T>* >(m_ptrs[name]); 
	out=tmp->pointer;
	
	return true;
	
      }
      else return false;

    }

    else return false;

  }
  
  
  
  template<typename T> void Set(std::string name,T in){
    std::stringstream stream;
    
    if(m_format==0 || m_format==2){
      boost::archive::binary_oarchive oa(stream);
           
      oa & in;
            
      stream.str(stream.str().replace(0,40,""));
            
    }
    else{
      boost::archive::text_oarchive oa(stream);
      oa & in;
      stream.str(stream.str().replace(0,28,""));
    }
    
    m_variables[name]=stream.str();
    if(m_typechecking) m_type_info[name]=typeid(in).name();

  }

  
  std::string* operator[](std::string key){
    return &m_variables[key];
  }
  

  template<typename T> void Set(std::string name,T* in, bool persist=true){
    
    std::stringstream stream;
    
    if(m_ptrs.count(name)>0){
      PointerWrapper<T> *tmp =  static_cast<PointerWrapper<T> *>(m_ptrs[name]);
      
      
      if(tmp->pointer!=in){
	delete m_ptrs[name];
	m_ptrs[name]=0;	
	
	m_ptrs[name]= new PointerWrapper<T>(in);
      }
    }

    else if(m_ptrs.count(name)==0)  m_ptrs[name]= new PointerWrapper<T>(in);
    
    
    if(persist){
      if(m_format==0 ||m_format==2){
	boost::archive::binary_oarchive oa(stream);
	oa & *in;
	stream.str(stream.str().replace(0,40,""));
      }
      else{
	boost::archive::text_oarchive oa(stream);
	oa & *in;
	stream.str(stream.str().replace(0,28,""));
      }
      
      
      m_variables[name]=stream.str();
      if(m_typechecking) m_type_info[name]=typeid(*in).name();
    }
  }
  
  
  
  template<typename T> void operator>>(T& obj){
    
    std::stringstream stream;
    stream<<"{";
    bool first=true;
    for (std::map<std::string,std::string>::iterator it=m_variables.begin(); it!=m_variables.end(); ++it){
      if (!first) stream<<",";
      stream<<"\""<<it->first<<"\":\""<< it->second<<"\"";
      first=false;
    }
    stream<<"}";
    
    obj=stream.str();
    
  }
  
 private:

  unsigned long currententry;
  unsigned long totalentries;
  std::string entryfile;
  std::string outfile;
  std::ifstream *file;
  std::ofstream *ofs;
  boost::archive::binary_iarchive* arch; 
  boost::archive::binary_oarchive* oarch;
  boost::iostreams::filtering_stream<boost::iostreams::output>* outfilter;
  boost::iostreams::filtering_stream<boost::iostreams::input>* infilter;    

  bool m_typechecking;
  int m_format;
  std::string m_archiveheader;
  std::map<std::string,std::string> m_variables;
  std::map<std::string,std::string> m_type_info;
  
  std::map<std::string,PointerWrapperBase*> m_ptrs;
  std::string tmpfile;
  
  friend class boost::serialization::access;
  
  void findheader(){
    std::string tmp="";
    std::stringstream stream;
    if(m_format==0 || m_format==2){
      boost::archive::binary_oarchive oa(stream);
      oa & tmp;
    }
    else{
      boost::archive::text_oarchive oa(stream);
      oa & tmp;
    }
    if(m_format==0 || m_format==2) m_archiveheader=stream.str().substr(0,40);
    else  m_archiveheader=stream.str().substr(0,28);
  }
  
  template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
      if(m_format!=2){
	ar & m_variables;
	if(m_typechecking)    ar & m_type_info;
      }
      else{
	if (oarch!=0){
	  Close();
	  std::ifstream tmp(outfile.c_str());
	  std::stringstream tmp2;
	  tmp2<<tmp.rdbuf();
	  std::string tmp3(tmp2.str());
	  ar & tmp3;
	  tmp.close();
	  remove (outfile.c_str());
	}
	else{
	  std::stringstream out;
	  out<<"/tmp/"<<time(0)<<(rand() % 999999999);
	  tmpfile=out.str();
	  std::ofstream tmp(tmpfile.c_str());
	  std::string tmp2;
	  ar & tmp2;
	  std::stringstream tmp3(tmp2);
	  tmp<<tmp2;
	  tmp.close();
	  m_format=2;
	  Initialise(tmpfile.c_str(),0);
	}
      
      }
      
    }
  
  
};

#endif

