#ifndef BoardReader_H
#define BoardReader_H

#include <string>
#include <iostream>

#include "Tool.h"

#include <stdlib.h>
#include <ctime>


class BoardReader: public Tool {

  //Simple class to simulate reading a card that has multiple channels saving an energy and time varable for each channel;

 public:

  BoardReader();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

  int Channels;



};


#endif
