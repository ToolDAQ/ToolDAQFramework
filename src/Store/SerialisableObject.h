#ifndef SERIALISABLEOBJECT_H
#define SERIALISABLEOBJECT_H

#include <string>

class SerialisableObject{

 public:

  virtual bool Print()=0;
  //  virtual ~SerialisableObject();
  
 protected:
  
  std::string type;
  bool serialise;
  std::string version;
  
 private:
  
  



};

#endif
