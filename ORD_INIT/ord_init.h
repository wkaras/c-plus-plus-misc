/*
Copyright (c) 2016 Walter William Karas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
The Ord_init class template helps to order the intialization of non-local
objects in the same executable but in different compilation units.  Suppose

A a;

and

B b;

are in different compilation units, but a's constructor depends on object b
being already initialized.  Then the declaration of b must be changed to

Ord_init<B> b;

The first use of b by class A's constructor must be a call to b.init().
Class B must have a parameterless constructor.  The object b will implicitly
convert to B &, and b() will return B &.

If class B does not have a parameterless constructor, and you don't wish
to make any changes to class B, you can derive an auxiliary class from B.
Even if b also has external dependencies.  For example:

extern Ord_init<Class_of_object_b_depends_on> object_b_depends_on;

struct B_aux_dep { B_aux_dep() { object_b_depends_on.init(); } };

struct B_aux : private B_aux_dep, public B
  { B_aux() : B(object_b_depends_on(), 666) { } };

Ord_init<B_aux> b;

As illustrated here, a reference to the object that b depends on must
be a parameter to the constructor of class B.

If the constructor for class B depends on another object, c say, then c
must be declared using Ord_init:

Ord_init<C> c;

even if c is defined in the same compilation unit as b.  B's constructor must
call c.init() before making any other use of c.

Ord_init takes a second class parameter called Traits, which defaults to
Ord_init_default_traits.  If using a version of C++ earlier than C++11,
Traits must define the public type Align_type.  Align_type must be a POD
type, with the same alignment requirements as the first type parameter to
Ord_init.  Traits must have a public member function cycle(uintptr_t)
(any return value is ignored).  This is called when a cyclical initialization
dependency of Ord_init objects is detected.

This facility does not provide any guarantees as to the order that objects
are destroyed in.

For Ord_init to work properly, all the objects defined with it, and those
that depend on them, must be constructed within the same execution thread.

*/

#ifndef ORD_INIT_20161107
#define ORD_INIT_20161107

#include <iostream>
#include <exception>

#if __cplusplus < 201100

#include <stdint.h>

#else

#include <cstdint>

#endif

#include "ios_flag_save.h"

struct Ord_init_default_traits
  {
    #if __cplusplus < 201100
    typedef int Align_type;
    #endif

    #if __cplusplus >= 201100
    typedef std::uintptr_t uintptr_t;
    #endif

    class Ord_init_cycle_exception : public std::exception
      {
      public:

        virtual const char * what() const
        #if __cplusplus >= 201100
        noexcept
        #else
        throw()
        #endif
        { return("Ord_init constructor encountered cycle"); }
      };

    IOS_FLAG_SAVE(Ifs)

    static void cycle(uintptr_t addr)
      {
        {
          Ifs sentry(std::cerr);

          std::cerr
            << "Cyclical initializaion dependencies for object at address 0x"
            << std::hex << addr << '\n';
        }

        throw Ord_init_cycle_exception();
      }
  };

template <class T, class Traits = Ord_init_default_traits>
class Ord_init
  {
  private:

    enum Status { Pre_init = 0, Init_in_progress, Init_done };

    // It's important that this member be initialized to zero
    // before any constructors are called.
    Status status;

    #if __cplusplus < 201100

    union
      {
        typename Traits::Align_type dummy;
        char raw[sizeof(T)];
      };

    #else

    alignas(T) char raw[sizeof(T)];

    typedef std::uintptr_t uintptr_t;

    #endif

  public:

    void init()
      {
        if (status == Init_done)
          return;

        if (status == Init_in_progress)
          Traits().cycle(reinterpret_cast<uintptr_t>(this));

        status = Init_in_progress;

        new(raw) T;

        status = Init_done;
      }

    Ord_init() { init(); }

    T & operator () () { return(* reinterpret_cast<T *>(raw)); }

    operator T & () { return((*this)()); }

    ~Ord_init()
      {
        if (status == Init_done)
          reinterpret_cast<T *>(raw)->~T();
      }

  // No copying except through T reference.
  #if __cplusplus < 201100

  private:
    Ord_init(const Ord_init &);
    Ord_init & operator = (const Ord_init &);

  #else

    Ord_init(const Ord_init &) = delete;
    Ord_init & operator = (const Ord_init &) = delete;

  #endif

  }; // end Ord_init

#endif // Include once.
