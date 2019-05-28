#ifndef SERIALISABLEOBJECT_H
#define SERIALISABLEOBJECT_H

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

/**
 * \class SerialisableObject
 *
 * An abstract base class for sustom calsses to inherit from to ensure version and type information are present, as well as a Print function and some form of sereialisation.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/

class SerialisableObject{
  
  friend class boost::serialization::access;
  
 public:
  
  virtual bool Print()=0; ///< Simple virtual Pritn function to ensure inhereted classes have one
  virtual ~SerialisableObject(){}; ///< Destructor
  bool serialise; ///< Denotes if the calss should be serialised or not when added to a BoostStore. 
  
 protected:
  
  std::string type; ///< String to store type of Tool
  std::string version; ///< String to store version of Tool
  
  /**
     Simple Boost serialise method to serialise the membervariables of a custom class. This shuld be expanded to include the custom classes variables
     @param ar Boost archive.
     @param version of the archive.
   */
  template<class Archive> void serialize(Archive & ar, const unsigned int version){  
    if(serialise){
      ar & type;
      ar & version;
    }
  }
  
  
 private:

};



#endif
