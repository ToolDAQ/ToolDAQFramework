#ifndef Filter_H
#define Filter_H

#include <string>
#include <iostream>

#include "Tool.h"

class Filter: public Tool {


 public:

  Filter();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:





};


#endif
