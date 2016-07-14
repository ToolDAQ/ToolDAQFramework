#ifndef BoardReader_H
#define BoardReader_H

#include <string>
#include <iostream>

#include "Tool.h"

#include <stdlib.h>
#include <ctime>

class BoardReader: public Tool {


 public:

  BoardReader();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:





};


#endif
