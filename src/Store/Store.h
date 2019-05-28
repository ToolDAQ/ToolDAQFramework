#ifndef STORE_H 
#define STORE_H

#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

/**
 * \class Store
 *
 * This class Is a dynamic data storeage class and can be used to store variables of any type listed by ASCII key. The storage of the varaible is in ASCII, so is inefficent for large numbers of entries.
 *
 * $Author: B.Richards $
 * $Date: 2019/05/28 10:44:00 $
 * Contact: b.richards@qmul.ac.uk
 */

class Store{

 public:

  Store(); ////< Sinple constructor

  void Initialise(std::string filename); ///< Initialises Store by reading in entries from an ASCII text file, when each line is a variable and its value in key value pairs.  @param filename The filepath and name to the input file.
  void JsonParser(std::string input); ///<  Converts a flat JSON formatted string to Store entries in the form of key value pairs.  @param input The input flat JSON string.
  void Print(); ///< Prints the contents of the Store.
  void Delete(); ///< Deletes all entries in the Store.


  /**
     Templated getter function for tore content. Assignment is templated and via reference.
     @param name The ASCII key that the variable in the Store is stored with.
     @param out The variable to fill with the value.
     @return Return value is true if varaible exists in the Store and correctly assigned to out and false if not.
  */
  template<typename T> bool Get(std::string name,T &out){
    
    if(m_variables.count(name)>0){

      std::stringstream stream(m_variables[name]);
      stream>>out;
      return true;
    }
    
    else return false;

  }

  /**
     Templated setter function to assign vairables in the Store.
     @param name The key to be used to store and reference the variable in the Store.
     @param in the varaible to be stored.
  */
  template<typename T> void Set(std::string name,T in){
    std::stringstream stream;
    stream<<in;
    m_variables[name]=stream.str();
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
     Allows streaming of a flat JASON formatted string of Store contents.
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

  std::map<std::string,std::string> m_variables;

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version)
  {
    ar & m_variables;
  }

};

#endif
