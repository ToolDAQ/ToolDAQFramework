#ifndef TOOL_H
#define TOOL_H

#include <string>

#include "Store.h"
#include "DataModel.h"

/**
 * \class Tool
 *
 * Abstract base class for Tools to inherit from. This allows a polymphic interface for the factor to use.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class Tool{
  
 public:
  
  virtual bool Initialise(std::string configfile,DataModel &data)=0; ///< virtual Initialise function that reads in the assigned config file and optain DataMoodel reference @param configfile Path and name of config file to read in. @param data Reference to DataModel. 
  virtual bool Execute()=0; ///< Virtual Execute function.
  virtual bool Finalise()=0; ///< Virtual Finalise function.
  virtual ~Tool(){}; ///< virtual destructor.
  
 protected:
  
  Store m_variables; ///< Store used to store configuration varaibles
  DataModel* m_data; ///< Pointer to transiant DataModel class
  Logging* m_log; ///< Pointer to logging class
  int m_verbose; ///< verbosity variable for direct logging level 
  MsgL ML(int messagelevel) {return MsgL(messagelevel,m_verbose);} ///< Function for setting logging level instream @param messagelevel the verboisty level at which to show the message. Checked against internal verbosity level.
  void MLC() {*(m_data->Log)<<MsgL(1,1);}  ///< Function for clearing logging level
  template <typename T>  void Log(T message, int messagelevel=1, int verbosity=1){m_data->Log->Log(message,messagelevel,verbosity);}
  template <typename T>  void Log(T message, int messagelevel){m_data->Log->Log(message,messagelevel,m_verbose);}  ///< Logging fuction for printouts. @param message Templated message string. @param messagelevel The verbosity level at which to show the message. Checked against internal verbosity level
  
  
 private:
  
  
  
  
  
};

#endif
