#ifndef POINTERWRAPPER_H
#define POINTERWRAPPER_H

class PointerWrapperBase{
  
 public:
  
  PointerWrapperBase(){}
  virtual ~PointerWrapperBase(){}
  
};


template <class T> class PointerWrapper : public PointerWrapperBase {
  
 public:
  
  T* pointer;
 PointerWrapper(T* inpointer): pointer(inpointer){}
  ~PointerWrapper(){delete pointer; pointer=0;};
  
};

#endif
