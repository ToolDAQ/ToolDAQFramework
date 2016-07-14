#ifndef RootWriter_H
#define RootWriter_H

#include <string>
#include <iostream>

#include "Tool.h"

class RootWriter: public Tool {


 public:

  RootWriter();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:





};


#endif
