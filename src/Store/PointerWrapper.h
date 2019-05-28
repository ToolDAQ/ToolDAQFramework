#ifndef POINTERWRAPPER_H
#define POINTERWRAPPER_H


/**
 * \class PointerWrapperBase
 *
 * This class is used to wrap pointers stored in a BoostStore. It gives a generic abstract base class to store pointers under
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class PointerWrapperBase{
  
 public:
  
  PointerWrapperBase(){} ///< Simple constructor
  virtual ~PointerWrapperBase(){} ///< virtual destructor
  
};

/**
 * \class PointerWrapper
 *
 * Teplated derived class of PointerWrapperBase to provide a plymorphic interface to stored poitner variables.
 *
 * $Author: B.Richards $
 * $Date: 2019/05/28 10:44:00 $
 * Contact: b.richards@qmul.ac.uk
 */
template <class T> class PointerWrapper : public PointerWrapperBase {
  
 public:
  
  T* pointer; ///< Pointer to the orignial obkect
 PointerWrapper(T* inpointer): pointer(inpointer){} ///< Constructor @param inpointer The pointer to be wrapped.
  ~PointerWrapper(){delete pointer; pointer=0;}; ///< Destructor.
  
};

#endif
