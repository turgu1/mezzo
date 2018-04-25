#ifndef NEW_HANDLER_SUPPORT_H
#define NEW_HANDLER_SUPPORT_H

/// The NewHandlerSupport class supplies mixin to simplify memory allocation process when an out of 
/// memory event occurs. It supplies a modified new operator that manage the C++ new_handler feature.
/// The class that inherit the mixin must supply it's own outOfMemory static method that will do the
/// appropriate actions when a memory allocation exception occurs.

template<class T>
class NewHandlerSupport {

 public:
  /// Declare a new_handler to get control on exceptions when the new operator is actived.
  static std::new_handler setNewHandler(std::new_handler p);

  /// The new operator replacing the C++ supplied operator
  static void * operator new(size_t size);

 private:
  /// Place holder for the class' new_handler
  static std::new_handler currentHandler;
};

template<class T>
std::new_handler NewHandlerSupport<T>::setNewHandler(std::new_handler p)
{
  std::new_handler old_handler = currentHandler;
  currentHandler = p;
  return old_handler;
}

template<class T>
void * NewHandlerSupport<T>::operator new(size_t size)
{
  std::new_handler globalHandler = std::set_new_handler(currentHandler);

  void *memory;

  try {
    memory = ::operator new(size);
  }
  catch (std::bad_alloc&) {
    std::set_new_handler(globalHandler);
    throw;
  }

  std::set_new_handler(globalHandler);
  
  return memory;
}

template<class T>
std::new_handler NewHandlerSupport<T>::currentHandler;

#endif
