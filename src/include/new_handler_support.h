// Notice
// ------
//
// This file is part of the Mezzo SoundFont2 Sampling Based Synthesizer:
//
//     https://github.com/turgu1/mezzo
//
// Simplified BSD License
// ----------------------
//
// Copyright (c) 2018, Guy Turcotte
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// The views and conclusions contained in the software and documentation are those
// of the authors and should not be interpreted as representing official policies,
// either expressed or implied, of the FreeBSD Project.
//

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
