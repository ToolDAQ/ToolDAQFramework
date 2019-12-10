#include "BoostStore.h"


bool BoostStore::Initialise(std::string filename, int type){

  file= new std::ifstream(filename.c_str());
  //  std::ifstream test(filename.c_str());
  if(type==0){
    // if(file->is_open(), std::ios::binary){
    if(file->is_open()){     
      boost::iostreams::filtering_stream<boost::iostreams::input> filter;
      filter.push(boost::iostreams::gzip_decompressor());
      filter.push(*file);
      
      if(!m_format){
	boost::archive::binary_iarchive ia(filter);
	ia & m_variables;
	if(m_typechecking)ia & m_type_info;
      }
      else if(m_format==1){
	boost::archive::text_iarchive ia(filter);
	ia & m_variables;
	if(m_typechecking)ia & m_type_info;
      }
      else if(m_format==2){
	entryfile=filename;
	infilter=new boost::iostreams::filtering_stream<boost::iostreams::input>;	
	infilter->push(boost::iostreams::gzip_decompressor());
	infilter->push(*file);
	arch = new boost::archive::binary_iarchive(*infilter);
	
	*arch & Header->m_variables;
       
	if(m_typechecking) *arch & Header->m_type_info;
	
	Header->Get("TotalEntries",totalentries);
 	m_variables.clear();
	m_type_info.clear();
	*arch & m_variables;
	if(m_typechecking) *arch & m_type_info;
	currententry=0;
	ofs=0;
      }
      
      if(m_format!=2){
	file->close();
	delete file;
	file=0;
      }
      return true;
    }
    else{
      std::cout<<"Error Bad Filename: Cannot open "<<filename<<std::endl;   
      return false;
    }
    
  }
  
  
  
  else{
    std::string line;
    
    if(file->is_open()){
      
      while (getline(*file,line)){
	if (line.size()>0){
	  if (line.at(0)=='#')continue;
	  std::string key;
	  std::string value;
	  std::stringstream stream(line);
	  if(stream>>key>>value){

	    std::stringstream stream;
	    if(!m_format){
	      boost::archive::binary_oarchive oa(stream);
	      oa & value;
	      stream.str(stream.str().replace(0,40,""));
	    }
	    else{
	      boost::archive::text_oarchive oa(stream);
	      oa & value;
	      stream.str(stream.str().replace(0,28,""));
	    }
	    m_variables[key]=stream.str();

	  }
	}	
      }
      
      file->close();
      return true;
    }
    
    else{
      std::cout<<"Error Bad Filename: Cannot open "<<filename<<std::endl;
      return false;
    }
  }
  
  
}


void BoostStore::Print(bool values){

  for (std::map<std::string,std::string>::iterator it=m_variables.begin(); it!=m_variables.end(); ++it){
   
    std::cout<< it->first << " => ";
    if(values) std::cout << it->second <<" :";
    std::cout<<" "<<m_type_info[it->first]<<std::endl;

  }

  for (std::map<std::string,PointerWrapperBase*>::iterator it=m_ptrs.begin(); it!=m_ptrs.end(); ++it){
    if(m_variables.count(it->first)==0){
      std::cout<< it->first << " => ";
      if(values) std::cout << it->second <<" :";
      std::cout<<" Pointer "<<std::endl;
    }
  }

}


void BoostStore::Delete(){ 
  
  m_variables.clear();
  m_type_info.clear();
  // for (std::map<std::string,void*>::iterator it=m_ptrs.begin(); it!=m_ptrs.end(); ++it){

  //  delete it->second;
  //   it->second=0;
    
  //}
  //m_ptrs.clear();

  for (std::map<std::string,PointerWrapperBase*>::iterator it=m_ptrs.begin(); it!=m_ptrs.end(); ++it){

    delete it->second;
    it->second=0;

  }
  m_ptrs.clear();

  
}


void BoostStore::Remove(std::string key){

  for (std::map<std::string,std::string>::iterator it=m_variables.begin(); it!=m_variables.end(); ++it){

    if(it->first==key){
      m_variables.erase(it);
      break;
    }
  }

  for (std::map<std::string,PointerWrapperBase*>::iterator it=m_ptrs.begin(); it!=m_ptrs.end(); ++it){

    if(it->first==key){
      delete it->second;
      it->second=0;
      m_ptrs.erase(it);
      break;
    }
  }
  

  if(m_typechecking){
    for (std::map<std::string,std::string>::iterator it=m_type_info.begin(); it!=m_type_info.end(); ++it){
      
      if(it->first==key){
	m_type_info.erase(it);
	break;
      }
    }
  }
  

}


void BoostStore::Save(std::string filename){
  
  if(m_format!=2){
    std::ofstream ofs(filename.c_str());
    boost::iostreams::filtering_stream<boost::iostreams::output> filter;
    filter.push(boost::iostreams::gzip_compressor());
    filter.push(ofs);
    Set("m_format",m_format);
    Set("m_typechecking",m_typechecking);
    
    if(!m_format){
      boost::archive::binary_oarchive oa(filter);
      oa & m_variables;
      if(m_typechecking)  oa & m_type_info;
    }
    else{
      boost::archive::text_oarchive oa(filter);
      oa & m_variables;
      if(m_typechecking)  oa & m_type_info;
    }
    filter.pop();
    ofs.close();
  }
  
  else {
    if(oarch==0){
      outfile=filename;
      std::stringstream tmp;
      tmp<<filename<<".data";
      ofs=new std::ofstream(tmp.str().c_str());  
      outfilter=new boost::iostreams::filtering_stream<boost::iostreams::output>;
      outfilter->push(boost::iostreams::gzip_compressor());
      outfilter->push(tmp);
      oarch=new boost::archive::binary_oarchive(*outfilter);
      *oarch & m_variables;
      if(m_typechecking)  *oarch & m_type_info;
      outfilter->pop();
      outfilter->push(*ofs);
    }
  
    totalentries++;

    *oarch & m_variables;
 
    if(m_typechecking)  *oarch & m_type_info;

  }
}

void BoostStore::JsonParser(std::string input){
  
  int type=0;
  std::string key;
  std::string value;
  
  for(std::string::size_type i = 0; i < input.size(); ++i) {
    
    if(input[i]=='\"')type++;
    else if(type==1)key+=input[i];
    else if(type==3)value+=input[i];
    else if(type==4){
      type=0;
      std::stringstream stream;
      if(!m_format){
	boost::archive::binary_oarchive oa(stream);
	oa & value;
	stream.str(stream.str().replace(0,40,""));
      }
      else{
	boost::archive::text_oarchive oa(stream);
	oa & value;
	stream.str(stream.str().replace(0,28,""));
      }
      m_variables[key]=stream.str();
      key="";
      value="";
    }
  }
}


std::string BoostStore::Type(std::string key){

  if(m_type_info.count(key)>0){
    if(m_typechecking) return m_type_info[key];
    else return "?";
  }
  else return "Not in Store";

}


bool BoostStore::GetEntry(unsigned long entry){
  
  if(entry<totalentries && m_format==2){

    if(entry==currententry)  return true;
    
    else if(entry>currententry){
      Delete();	
      for(unsigned long i=currententry; i<entry; i++){
	
	m_variables.clear();
	m_type_info.clear();
	*arch & m_variables;
	if(m_typechecking) *arch & m_type_info;
	currententry=i+1;
	
      }
      return true;
      
    }
    else if(entry<currententry){

      Delete();
      delete arch;
      arch=0;
      file->close();
      file=0;
      delete infilter;
      infilter=0;

      file= new std::ifstream(entryfile.c_str());
    
      if(file->is_open(), std::ios::binary){

	infilter=new boost::iostreams::filtering_stream<boost::iostreams::input>;
        infilter->push(boost::iostreams::gzip_decompressor());
        infilter->push(*file);
        arch = new boost::archive::binary_iarchive(*infilter);

	m_variables.clear();
	m_type_info.clear();
	*arch & m_variables;
	//	if(m_typechecking) *arch & m_type_info;	
	Get("TotalEntries",totalentries);
	m_variables.clear();
	m_type_info.clear();
	currententry=0;
	
	for(unsigned long i=0; i<entry+1; i++){
	  m_variables.clear();
	  m_type_info.clear();
	  *arch & m_variables;
	  if(m_typechecking) *arch & m_type_info;
	  currententry=i;
	}
	return true;
	
      }
      else return false;
    }
    else return false;
  }
  else return false;
  
}	  


bool BoostStore::Close(){

   if(m_format==2){
     if(arch!=0){
       delete arch; 
       arch=0;        
       file->close();      
       file=0;
       delete infilter;
       infilter=0;
     }
     
     if(oarch!=0){
       delete oarch;     
       outfilter->pop();       
       oarch=0;
       ofs->close();
       ofs=0;
       delete outfilter;
       outfilter=0;       

       std::ofstream out(outfile.c_str());
       boost::iostreams::filtering_stream<boost::iostreams::output> filter;
       filter.push(boost::iostreams::gzip_compressor());
       filter.push(out);
       boost::archive::binary_oarchive* oa =new boost::archive::binary_oarchive(filter);
       Header->Set("TotalEntries",totalentries);
       Header->Set("m_format",m_format);
       Header->Set("m_typechecking",m_typechecking);
       *oa & Header->m_variables;
       if(m_typechecking)  *oa & Header->m_type_info;
       delete oa;
       filter.pop();
       
       
       std::stringstream tmp;
       tmp<<outfile<<".data";
       std::ifstream data(tmp.str().c_str());
       out<<data.rdbuf();
       out.close();
       data.close();
       tmp.str("");
       tmp<< "rm "<<outfile<<".data";
       remove(tmp.str().c_str());
       //system(tmp.str().c_str());
       
     }
     
     return true;
     
   }
   
   else return false;
   
   
}

bool BoostStore::Has(std::string key){

  if(m_variables.count(key)>0) return true;
  else return false;

}


BoostStore::~BoostStore() {
  Delete();
  remove(tmpfile.c_str());

}
