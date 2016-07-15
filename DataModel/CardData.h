#ifndef CARDDATA_H
#define CARDDATA_H

#include <string>
#include <iostream>
#include <vector>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>

struct CardData{

public:

  int Channels;
  int CardID;
  std::vector<int> Channel;
  std::vector<int> Energy;
  std::vector<int> Time;
  
private:
friend class boost::serialization::access;
  
  template<class Archive> void serialize(Archive & ar, const unsigned int version){
    
    ar & Channels;
    ar & CardID;
    ar & Channel;
    ar & Energy;
    ar & Time;
    
  }

};

#endif
