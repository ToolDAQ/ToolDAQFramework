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

/**
 * \class BoostStore
 *
 * This class Is a dynamic data storeage class and can be used to store variables of any type listed by ASCII key. The storage can be in binary (using boost archives) or ASCII with typechecking information included if reqired. A Boost Store can also be used to store multi event like data sets withe unique stores for each event and a header. Any standard and STL container can be stored as a variable as well as poitners and custom classes/ objects can also be stored by including boost serialisation functions and freindships.
 *
 * $Author: B.Richards $
 * $Date: 2019/05/28 10:44:00 $
 * Contact: b.richards@qmul.ac.uk
 */

class BoostStore{
  
 public:
  
  /**
     Standard constructor of BoostStore
     @param typechecking This is a bool flag for turning on and off the type checking on stored objects. If true data of the varaible types will be stored and used to ensure the same type is requested by the getters later. If false varaibles will be attempted to be interpreted as the type requested. 
     @param format This denotes the underlying storage type and mechanism for storage. 0 = binary backend, 1 = ASCII backend, 2 = multievent based binary backend.   
   */
 BoostStore(bool typechecking=true, int format=0): m_typechecking(typechecking),m_format(format) {findheader();oarch=0;arch=0;totalentries=0;
    if(m_format==2){
      Header = new BoostStore(typechecking,0);
    }
    srand (time(NULL)); 
  } // format 0=binary, 1=text, 2=multievent.

  /**
     Initialised constrcutor of a BoostStore. This allows a boost store to be made directly from a map of strings to strings.
     @param invariables The map of strings to strings used for initialising the BoostStore.
  */
 BoostStore(std::map<std::string,std::string> invariables):m_variables(invariables){findheader();oarch=0;arch=0;}
  /**
     Initialised constrcutor of a BoostStore. This allows a boost store to be made directly from a map of strings to strings, with a secondary map of strings to strings as the type info.
     @param invariables The map of strings to strings used for initialising the BoostStore.
     @param ininfo The map of strings to strings used for initialising the type checking of the BoostStore.
  */
 BoostStore(std::map<std::string,std::string> invariables, std::map<std::string,std::string> ininfo):m_variables(invariables), m_type_info(ininfo){findheader();oarch=0;arch=0;}
  bool Initialise(std::string filename, int type=0); ///< Initialise the BoostStore from a file. @param filename the path and name of the input file. @param type the input file type, 0=boost archive, 1 = ASCII config file 
  void JsonParser(std::string input); ///< Converts a flat JSON formatted string to Store entries in the form of key value pairs.  @param input The input flat JSON string. 
  void Print(bool values=true); ///< Prints the contents of the BoostStore. @param values If true values and keys are printed. If false just keys are printed
  void Delete(); ///< Deletes all entries in the BoostStore.
  void Remove(std::string key); ///< Removes a single entry from the BoostStore. @param key The key of the entry to remove. 
  void Save(std::string filename="Output"); ///< Saves the boost store to disk. @param filename The output name and path of where to save the BoostStore contents.
  std::string Type(std::string key); ///< Queries the type of an entry if type checking is turned on. @param key The key of the entry to check. @return A string encoding the type info.
  bool GetEntry(unsigned long entry); ///< Used to retrun an entry if a multievent BoostStore. @param entry The entry to return @return True if sucsessfull, flase if not.
  bool Close(); ///< Closes the the currently open file. Multievent stores do not load all entries into memory but rather hanf on to a file stream and read entries as needed. This will close the stream. @return true if sucsessfull, false if not. 
  bool Has(std::string key); ///< Queries if entry exists in a BoostStore. @param key is the key of the varaible to look up. @return true if varaible is present in the store, false if not. 
  ~BoostStore(); ///< Destructor for BoostStore
  BoostStore *Header; ///< Pointer to header BoostStore (only available in multi event BoostStore). This can be used to access and assign header varaibles jsut like a standard non multi event store.
  
  /**
     Templated getter function for BoostStore content. Assignment is templated and via reference.
     @param name The ASCII key that the variable in the BoostStore is stored with.
     @param out The variable to fill with the value.
     @return Return value is true if varaible exists in the Store and false if not or wrong type.
  */
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

  /**
     Templated getter function for pointer to BoostStore content. Assignment is templated and the object is created on first the heap and a pointer to it assigned. Latter requests will return a pointer to the first object. Bote the pointer is to an indipendant instance of the stored object and so changing its value without using a Setter will not effect the stored value. 
     @param name The ASCII key that the variable in the BoostStore is stored with.
     @param out The pointer to assign.
     @return Return value is true if varaible exists in the Store and false if not or wrong type.
  */
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
  
  
  /**
     Templated setter function to assign vairables in the BoostStore.
     @param name The key to be used to store and reference the variable in the BoostStore.
     @param in the varaible to be stored.
  */
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

  /**
     Returns string pointer to Store element.
     @param key The key of the string pointer to return.
     @return a pointer to the string version of the value within the Store.
  */  
  std::string* operator[](std::string key){
    return &m_variables[key];
  }
  
  /**
     Templated setter function to assign vairables in the BoostStore from pointers.
     @param name The key to be used to store and reference the variable in the BoostStore.
     @param in A pointer to the object to be stored. The function will store the value the pointer poitns to in the archive and keep hold of the pointer to return to Get calls later.
     @param persist Indicates if the object that is being pointed to is stored or not. If not only the pointer is retained for later use but not in the archive and so will not be saved with the BoostStore on save calls.
  */
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
  
  
  /**
     Allows streaming of a flat JASON formatted string of BoostStore contents.
  */
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

