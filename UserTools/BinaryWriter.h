#ifndef BinaryWriter_H
#define BinaryWriter_H

#include <string>
#include <iostream>

#include "Tool.h"

class BinaryWriter: public Tool {


 public:

  BinaryWriter();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

  std::string outfile;
  std::ofstream *ofs;
  boost::archive::text_oarchive *oa;

};


#endif
